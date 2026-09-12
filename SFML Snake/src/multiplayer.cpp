#include "multiplayer.hpp"
#include "Protocol.hpp"

bool MultiplayerClient::connect(const std::string& serverAddress, uint16_t port, const std::string& username)
{
	lastError.clear();

	auto address = sf::IpAddress::resolve(serverAddress);
	if (!address)
	{
		lastError = "Server address can't be resolved";
		return false;
	}

	if (socket.connect(*address, port, sf::seconds(3.f)) != sf::Socket::Status::Done)
	{
		lastError = "Socket can't be opened";
		return false;
	}

	sf::Packet joinPacket;
	joinPacket << static_cast<std::uint8_t>(Protocol::MessageType::Join);
	joinPacket << username;

	if (socket.send(joinPacket) != sf::Socket::Status::Done)
	{
		lastError = "Packet can't be sent";
		resetSocket();
		return false;
	}

	sf::SocketSelector selector;
	selector.add(socket);
	if (!selector.wait(sf::seconds(3.f)))
	{
		lastError = "No response from the server";
		resetSocket();
		return false;
	}

	sf::Packet response;
	if (socket.receive(response) != sf::Socket::Status::Done)
	{
		lastError = "No response from the server";
		resetSocket();
		return false;
	}

	std::uint8_t rawType = 0;
	response >> rawType;
	auto type = static_cast<Protocol::MessageType>(rawType);

	if (type == Protocol::MessageType::JoinRejected)
	{
		std::string reason;
		response >> reason;
		lastError = reason;
		resetSocket();
		return false;
	}

	if (type != Protocol::MessageType::PlayerList)
	{
		lastError = "Expected PlayerList, didn't receive it";
		resetSocket();
		return false;
	}

	applyPlayerList(response);

	socket.setBlocking(false);
	connected = true;
	return true;
}

void MultiplayerClient::resetSocket()
{
	socket.disconnect();
	socket = sf::TcpSocket();
}

void MultiplayerClient::applyPlayerList(sf::Packet& packet)
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

void MultiplayerClient::disconnect()
{
	resetSocket();
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
				applyPlayerList(packet);
		}
		else if (status == sf::Socket::Status::NotReady)
			break;
		else
		{
			resetSocket();
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

const std::string& MultiplayerClient::getLastError() const
{
	return lastError;
}
