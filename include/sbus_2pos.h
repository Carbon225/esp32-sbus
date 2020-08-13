#ifndef _COMPONENT_SBUS_2POS_H_
#define _COMPONENT_SBUS_2POS_H_

#include "sbus_raw_channel.h"

namespace SBUSChannels
{
	class Switch2Pos : private Raw
	{
	public:
		enum State
		{
			Down,
			Up
		};
		
		Switch2Pos(const SBUS &sbus, int chNum, int onLevel);
		State state() const;
		bool isUp() const;
		operator bool() const;
		
	private:
		const int _onLevel;
	};
}

#endif // !_COMPONENT_SBUS_2POS_H_
