#include <SFML/Network.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "Protocol.hpp"

struct Client
{
	sf::TcpSocket socket;
	std::string username;
};

static void broadcastPlayerList(const std::vector<std::unique_ptr<Client>>& clients)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::PlayerList);
	packet << static_cast<std::uint32_t>(clients.size());
	for (const auto& client : clients)
		packet << client->username;

	for (const auto& client : clients)
		(void)client->socket.send(packet);
}

int main()
{
	sf::TcpListener listener;
	if (listener.listen(Protocol::PORT) != sf::Socket::Status::Done)
	{
		std::cerr << "Failed to listen on port " << Protocol::PORT << "\n";
		return 1;
	}

	std::cout << "Snake server listening on port " << Protocol::PORT << "\n";

	sf::SocketSelector selector;
	selector.add(listener);

	std::vector<std::unique_ptr<Client>> clients;

	while (true)
	{
		if (!selector.wait())
			continue;

		if (selector.isReady(listener))
		{
			auto client = std::make_unique<Client>();
			if (listener.accept(client->socket) == sf::Socket::Status::Done)
			{
				selector.add(client->socket);
				clients.push_back(std::move(client));
			}
		}

		for (size_t i = 0; i < clients.size();)
		{
			Client& client = *clients[i];
			bool removed = false;

			if (selector.isReady(client.socket))
			{
				sf::Packet packet;
				sf::Socket::Status status = client.socket.receive(packet);

				if (status == sf::Socket::Status::Done)
				{
					std::uint8_t rawType = 0;
					packet >> rawType;

					if (static_cast<Protocol::MessageType>(rawType) == Protocol::MessageType::Join)
					{
						std::string username;
						packet >> username;
						client.username = username;

						std::string address = "unknown";
						if (auto remote = client.socket.getRemoteAddress())
							address = remote->toString();

						std::cout << "[+] \"" << username << "\" connected from " << address << "\n";

						broadcastPlayerList(clients);
					}
				}
				else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error)
				{
					std::cout << "[-] \"" << client.username << "\" disconnected\n";

					selector.remove(client.socket);
					clients.erase(clients.begin() + static_cast<std::ptrdiff_t>(i));
					removed = true;

					broadcastPlayerList(clients);
				}
			}

			if (!removed)
				++i;
		}
	}
}
