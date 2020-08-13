#include "sbus.h"

#define mapRange(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

#include "esp_log.h"
static const char* TAG = "SBUS";

SBUS::SBUS()
{

}

SBUS::~SBUS()
{

}

esp_err_t SBUS::install(uart_port_t uartNum, int gpioNum, bool inv)
{
	switch (uartNum) {
	case UART_NUM_0:
		_uart = &UART0;
		break;

	case UART_NUM_1:
		_uart = &UART1;
		break;

	case UART_NUM_2:
		_uart = &UART2;
		break;

	default:
		ESP_LOGE(TAG, "Invalid UART number");
		return ESP_ERR_INVALID_ARG;
	}

	_queueHandle = xQueueCreate(SBUS_QUEUE_SIZE, sizeof(sbus_packet_t));
	if (!_queueHandle) {
		ESP_LOGE(TAG, "Creating SBUS queue failed");
		return ESP_FAIL;
	}

	xTaskCreatePinnedToCore(&sbusQueueHandler, "sbus_queue", 2048, this, 12, &_taskHandle, 1);

	const uart_config_t uart_config = {
		.baud_rate = 100000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_EVEN,
		.stop_bits = UART_STOP_BITS_2,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};

	esp_err_t err;

	err = uart_param_config(uartNum, &uart_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "uart_param_config failed");
		return err;
	}

	err = uart_set_pin(uartNum, UART_PIN_NO_CHANGE, gpioNum, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "uart_set_pin failed");
		return err;
	}

	err = uart_isr_register(uartNum,
		&uartIsrHandler,
		this,
		ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM,
		&_isrHandle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "uart_isr_register failed");
		return err;
	}

	const uart_intr_config_t intr_config = {
		.intr_enable_mask = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA,
		.rx_timeout_thresh = SBUS_RX_TOUT,
		.rxfifo_full_thresh = SBUS_PACKET_SIZE
	};

	err = uart_intr_config(uartNum, &intr_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "uart_intr_config failed");
		return err;
	}

	return err;
}

uint16_t SBUS::channel(int chNum) const
{
	return _channels[chNum];
}

esp_err_t SBUS::onError(sbus_error_cb fn)
{
	_errorCallback = fn;
	return ESP_OK;
}

esp_err_t SBUS::onPacket(sbus_packet_cb fn)
{
	_packetCallback = fn;
	return ESP_OK;
}


void IRAM_ATTR SBUS::uartIsrHandler(void *arg)
{
	static uint8_t buf[SBUS_FIFO_SIZE];

	SBUS *sbus = (SBUS*)arg;
	uart_dev_t *uart = sbus->_uart;

	uint32_t rxfifo_cnt = uart->status.rxfifo_cnt;
	for (int i = 0; i < rxfifo_cnt; i++) {
		buf[i] = uart->fifo.rw_byte;
	}
	
	if (uart->int_st.val == 0) {
		return;
	}

	sbus_packet_t packet = {
		.event = SBUS_ERROR,
		.channels = NULL,
		.ch17 = 0,
		.ch18 = 0,
		.failsafe = true,
		.frameLost = true
	};

	if (uart->int_st.rxfifo_full) {
		uart->int_clr.rxfifo_full = 1;

		if (decodeSBUS(buf, sbus->_channels, &packet.failsafe, &packet.frameLost, &packet.ch17, &packet.ch18) == ESP_OK) {
			packet.channels = sbus->_channels;
			packet.event = SBUS_PACKET;
		}
		else {
			packet.event = SBUS_ERROR;
		}
	}
	else if (uart->int_st.rxfifo_tout) {
		uart->int_clr.rxfifo_tout = 1;
		packet.event = SBUS_DESYNC;
	}
	
	uart->int_clr.val = UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR;

	BaseType_t task_woken = pdFALSE;
	xQueueSendFromISR(sbus->_queueHandle, &packet, &task_woken);
	if (task_woken)
	{
		portYIELD_FROM_ISR();
	}
	
	if (packet.event == SBUS_PACKET)
	{
		sbus->_lastPacket = xTaskGetTickCountFromISR();
	}
}

void SBUS::sbusQueueHandler(void *pvParams)
{
	SBUS *sbus = (SBUS*)pvParams;

	sbus_packet_t packet;
	while (true) {
		if (xQueueReceive(sbus->_queueHandle, &packet, portMAX_DELAY)) {
			switch (packet.event) {
			case SBUS_PACKET:
				if (sbus->_packetCallback) {
					sbus->_packetCallback(&packet);
				}
				break;

			case SBUS_DESYNC:
				ESP_LOGW(TAG, "SBUS desync");
				break;

			case SBUS_ERROR:
				ESP_LOGE(TAG, "SBUS packet error");
				break;
			}
		}
		portYIELD();
	}
	vTaskDelete(NULL);
}

esp_err_t decodeSBUS(uint8_t packet[25],
	uint16_t channels[16],
	bool *failsafe,
	bool *frame_lost,
	bool *ch17,
	bool *ch18)
{
	if (!packet || !channels || !frame_lost || !failsafe) {
		return ESP_ERR_INVALID_ARG;
	}
	if (*packet != SBUS_HEADER || packet[24] != SBUS_END) {
		return ESP_FAIL;
	}

	uint8_t *payload = packet + 1;
	channels[0]  = (uint16_t)((payload[0]    | payload[1] << 8)                    & 0x07FF);
	channels[1]  = (uint16_t)((payload[1] >> 3 | payload[2] << 5)                    & 0x07FF);
	channels[2]  = (uint16_t)((payload[2] >> 6 | payload[3] << 2 | payload[4] << 10)    & 0x07FF);
	channels[3]  = (uint16_t)((payload[4] >> 1 | payload[5] << 7)                    & 0x07FF);
	channels[4]  = (uint16_t)((payload[5] >> 4 | payload[6] << 4)                    & 0x07FF);
	channels[5]  = (uint16_t)((payload[6] >> 7 | payload[7] << 1 | payload[8] << 9)     & 0x07FF);
	channels[6]  = (uint16_t)((payload[8] >> 2 | payload[9] << 6)                    & 0x07FF);
	channels[7]  = (uint16_t)((payload[9] >> 5 | payload[10] << 3)                    & 0x07FF);
	channels[8]  = (uint16_t)((payload[11]   | payload[12] << 8)                    & 0x07FF);
	channels[9]  = (uint16_t)((payload[12] >> 3 | payload[13] << 5)                    & 0x07FF);
	channels[10] = (uint16_t)((payload[13] >> 6 | payload[14] << 2 | payload[15] << 10)   & 0x07FF);
	channels[11] = (uint16_t)((payload[15] >> 1 | payload[16] << 7)                    & 0x07FF);
	channels[12] = (uint16_t)((payload[16] >> 4 | payload[17] << 4)                    & 0x07FF);
	channels[13] = (uint16_t)((payload[17] >> 7 | payload[18] << 1 | payload[19] << 9)    & 0x07FF);
	channels[14] = (uint16_t)((payload[19] >> 2 | payload[20] << 6)                    & 0x07FF);
	channels[15] = (uint16_t)((payload[20] >> 5 | payload[21] << 3)                    & 0x07FF);

	*failsafe = packet[23] >> 3 & 1;
	*frame_lost = packet[23] >> 2 & 1;
    
	if (ch17) {
		*ch17 = packet[23] & 1;
	}
	if (ch18) {
		*ch18 = packet[23] >> 1 & 1;
	}

	return ESP_OK;
}

TickType_t SBUS::lastPacket() const
{
	return _lastPacket;
}
