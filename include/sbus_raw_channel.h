#ifndef _COMPONENT_SBUS_CHANNEL_RAW_H_
#define _COMPONENT_SBUS_CHANNEL_RAW_H_

#include "sbus.h"

namespace SBUSChannels
{
	class Raw
	{
	public:
		Raw(const SBUS &sbus, int chNum);
		int32_t value() const;
		operator int32_t() const;
	
	private:
		const int _num;
		const SBUS &_sbus;
	};
}

#endif // !_COMPONENT_SBUS_CHANNEL_RAW_H_
