#include "multiplayer.hpp"
#include "Protocol.hpp"

bool MultiplayerClient::connect(const std::string& serverAddress, uint16_t port, const std::string& username)
{
	auto address = sf::IpAddress::resolve(serverAddress);
	if (!address)
		return false;

	if (socket.connect(*address, port, sf::seconds(3.f)) != sf::Socket::Status::Done)
		return false;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::Join);
	packet << username;

	if (socket.send(packet) != sf::Socket::Status::Done)
	{
		socket.disconnect();
		return false;
	}

	socket.setBlocking(false);
	players.clear();
	connected = true;
	return true;
}

void MultiplayerClient::disconnect()
{
	socket.disconnect();
	connected = false;
	players.clear();
}

void MultiplayerClient::update()
{
	if (!connected)
		return;

	while (true)
	{
		sf::Packet packet;
		sf::Socket::Status status = socket.receive(packet);

		if (status == sf::Socket::Status::Done)
		{
			std::uint8_t rawType = 0;
			packet >> rawType;

			if (static_cast<Protocol::MessageType>(rawType) == Protocol::MessageType::PlayerList)
			{
				std::uint32_t count = 0;
				packet >> count;

				std::vector<std::string> updated;
				updated.reserve(count);
				for (std::uint32_t i = 0; i < count; ++i)
				{
					std::string name;
					packet >> name;
					updated.push_back(std::move(name));
				}
				players = std::move(updated);
			}
		}
		else if (status == sf::Socket::Status::NotReady)
			break;
		else
		{
			connected = false;
			players.clear();
			break;
		}
	}
}

bool MultiplayerClient::isConnected() const
{
	return connected;
}

const std::vector<std::string>& MultiplayerClient::getPlayers() const
{
	return players;
}
