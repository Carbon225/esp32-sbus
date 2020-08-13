#include "sbus_2pos.h"

using namespace SBUSChannels;

Switch2Pos::Switch2Pos(const SBUS &sbus, int chNum, int onLevel)
	: Raw(sbus, chNum)
	, _onLevel(onLevel)
{
}


Switch2Pos::State Switch2Pos::state() const
{
	int32_t value = Raw::value();
	if (value > _onLevel)
		return State::Up;
	else
		return State::Down;
}

bool Switch2Pos::isUp() const
{
	return state() == Switch2Pos::Up;
}

Switch2Pos::operator bool() const
{
	return isUp();
}