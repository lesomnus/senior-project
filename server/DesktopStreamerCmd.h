#pragma once

#include <cstdint>

namespace DesktopStreamerCmd{
enum cmd: uint8_t{
	DISCONNECT,		// client disconnect
	READY_TO_RCV,	// client ready to receive 
	META_SIZE,		// metadata: client screen size
					//	8-bit for code, 16-bit for width and height size[px]
					//	total 8 + 16 + 16 = 40-bit(= 5-byte)
					//	0x<code>'wwww'hhhh;
	_SIZE			// number of commands
};
}