#ifndef _COMPONENT_SBUS_H_
#define _COMPONENT_SBUS_H_

#include "driver/uart.h"

#define SBUS_PACKET_SIZE 25
#define SBUS_HEADER 0x0f
#define SBUS_END 0x00

#define SBUS_FIFO_SIZE SBUS_PACKET_SIZE
#define SBUS_QUEUE_SIZE 64
#define SBUS_RX_TOUT 5

#define SBUS_CHANNEL_MIN CONFIG_SBUS_CHANNEL_MIN
#define SBUS_CHANNEL_MAX CONFIG_SBUS_CHANNEL_MAX

enum sbus_event_t
{
	SBUS_PACKET,
	SBUS_DESYNC,
	SBUS_ERROR
};

struct sbus_packet_t
{
	sbus_event_t event;
	uint16_t *channels;
	bool ch17, ch18;
	bool failsafe;
	bool frameLost;
};

typedef void(*sbus_error_cb)(bool failsafe, bool frameLost);
typedef void(*sbus_packet_cb)(sbus_packet_t*);

esp_err_t decodeSBUS(uint8_t packet[25],
	uint16_t channels[16],
	bool *failsafe,
	bool *frame_lost,
	bool *ch17 = NULL,
	bool *ch18 = NULL);

class SBUS
{
public:
	SBUS();
	~SBUS();

	esp_err_t install(uart_port_t uartNum, int gpioNum, bool inv = true);
	uint16_t channel(int chNum) const;
	TickType_t lastPacket() const;
	esp_err_t onError(sbus_error_cb fn);
	esp_err_t onPacket(sbus_packet_cb fn);

private:
	uart_dev_t *_uart = NULL;
	QueueHandle_t _queueHandle = NULL;
	uart_isr_handle_t _isrHandle = NULL;
	TaskHandle_t _taskHandle = NULL;

	uint16_t _channels[16] = { 0 };
	bool _failsafe = false;
	bool _frameLost = false;
	TickType_t _lastPacket = 0;

	sbus_error_cb _errorCallback = NULL;
	sbus_packet_cb _packetCallback = NULL;
    
	static void sbusQueueHandler(void *pvParams);
	static void IRAM_ATTR uartIsrHandler(void *arg);
};

#endif
