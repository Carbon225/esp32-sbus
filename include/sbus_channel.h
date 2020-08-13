#ifndef _COMPONENT_SBUS_CHANNEL_H_
#define _COMPONENT_SBUS_CHANNEL_H_

#include "sbus_raw_channel.h"

namespace SBUSChannels
{
	class Analog : private Raw
	{
	public:
		Analog(const SBUS &sbus, int chNum, float min, float max);
		float value() const;
		operator float() const;
	
	private:
		const float _min;
		const float _max;
	};
}

#endif // !_COMPONENT_SBUS_CHANNEL_H_
