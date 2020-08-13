#ifndef _COMPONENT_SBUS_3POS_H_
#define _COMPONENT_SBUS_3POS_H_

#include "sbus_raw_channel.h"

namespace SBUSChannels
{
	class Switch3Pos : private Raw
	{
	public:
		enum State
		{
			Down,
			Mid,
			Up
		};
		
		Switch3Pos(const SBUS &sbus, int chNum, int level1, int level2);
		State state() const;
		operator State() const;
		
	private:
		const int _level1;
		const int _level2;
	};
}

#endif // !_COMPONENT_SBUS_3POS_H_
