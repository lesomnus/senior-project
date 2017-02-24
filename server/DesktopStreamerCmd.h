#pragma once

#include <cstdint>

namespace DesktopStreamerCmd{
enum: uint8_t{
	_UNDF,
	READY_TO_RCV,	// client ready to receive 
	_SIZE			// number of commands
};
}