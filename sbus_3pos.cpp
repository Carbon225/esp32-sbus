#include "sbus_3pos.h"

using namespace SBUSChannels;

Switch3Pos::Switch3Pos(const SBUS &sbus, int chNum, int level1, int level2)
	: Raw(sbus, chNum)
	, _level1(level1)
	, _level2(level2)
{
}


Switch3Pos::State Switch3Pos::state() const
{
	int32_t value = Raw::value();
	if (value > _level2)
		return State::Up;
	else if (value > _level1)
		return State::Mid;
	else
		return State::Down;
}

Switch3Pos::operator Switch3Pos::State() const
{
	return state();
}
