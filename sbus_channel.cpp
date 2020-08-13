#include "sbus_channel.h"

#define mapRange(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

using namespace SBUSChannels;

Analog::Analog(const SBUS &sbus, int chNum, float min, float max)
	: Raw(sbus, chNum)
	, _min(min)
	, _max(max)
{
	
}

float Analog::value() const
{
	return  mapRange((float) Raw::value(), (float) SBUS_CHANNEL_MIN, (float) CONFIG_SBUS_CHANNEL_MAX, _min, _max);
}

Analog::operator float() const
{
	return value();
}
