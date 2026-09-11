#pragma once
#include <cstdint>
#include <string>

namespace Protocol
{
	const uint16_t PORT = 55001;
	const std::string ServerAddress = "127.0.0.1";

	enum class MessageType : std::uint8_t
	{
		Join,   
		PlayerList,
	};
}
