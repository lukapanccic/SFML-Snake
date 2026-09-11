#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include <string>
#include <vector>

class MultiplayerClient
{
	sf::TcpSocket socket;
	bool connected = false;
	std::vector<std::string> players;

public:
	bool connect(const std::string& serverAddress, uint16_t port, const std::string& username);
	void disconnect();
	void update();
	bool isConnected() const;
	const std::vector<std::string>& getPlayers() const;
};
