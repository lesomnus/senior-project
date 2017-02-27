#pragma once

#include <cstdint>

namespace DesktopStreamerCmd{
enum cmd: uint8_t{
	DISCONNECT,		// client disconnect
	READY_TO_RCV,	// client ready to receive 
	_SIZE			// number of commands
};
}