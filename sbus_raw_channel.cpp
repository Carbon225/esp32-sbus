#include "sbus_raw_channel.h"

using namespace SBUSChannels;

Raw::Raw(const SBUS &sbus, int chNum)
	: _num(chNum)
	, _sbus(sbus)
{
	
}

int32_t Raw::value() const
{
	return _sbus.channel(_num);
}

Raw::operator int32_t() const
{
	return value();
}