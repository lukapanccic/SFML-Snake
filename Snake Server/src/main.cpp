#include <SFML/Network.hpp>
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "Protocol.hpp"

struct Client
{
	sf::TcpSocket socket;
	std::string username;
	std::uint32_t id = 0;
};

struct Invite
{
	std::uint32_t fromId;
	std::uint32_t toId;
	sf::Clock clock;
};

struct Room
{
	std::uint32_t masterId;
	std::uint32_t guestId;
	std::uint32_t seed;
};

static std::vector<std::unique_ptr<Client>> clients;
static std::vector<Invite> invites;
static std::vector<Room> rooms;
static std::mt19937 seedGenerator(std::random_device{}());

static Client* findClientById(std::uint32_t id)
{
	for (auto& c : clients)
		if (c->id == id)
			return c.get();
	return nullptr;
}

static Client* findClientByUsername(const std::string& username)
{
	for (auto& c : clients)
		if (c->username == username)
			return c.get();
	return nullptr;
}

static bool isBusy(std::uint32_t id)
{
	for (auto& inv : invites)
		if (inv.fromId == id || inv.toId == id)
			return true;
	for (auto& room : rooms)
		if (room.masterId == id || room.guestId == id)
			return true;
	return false;
}

static Room* findRoomByMaster(std::uint32_t id)
{
	for (auto& room : rooms)
		if (room.masterId == id)
			return &room;
	return nullptr;
}

static Room* findRoomInvolving(std::uint32_t id)
{
	for (auto& room : rooms)
		if (room.masterId == id || room.guestId == id)
			return &room;
	return nullptr;
}

static void removeRoom(Room* room)
{
	rooms.erase(rooms.begin() + (room - rooms.data()));
}

static void sendTo(std::uint32_t id, sf::Packet& packet)
{
	if (Client* c = findClientById(id))
		(void)c->socket.send(packet);
}

static void broadcastPlayerList()
{
	std::vector<std::string> usernames;
	for (const auto& client : clients)
		if (!client->username.empty())
			usernames.push_back(client->username);

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::PlayerList);
	packet << static_cast<std::uint32_t>(usernames.size());
	for (const auto& name : usernames)
		packet << name;

	for (const auto& client : clients)
		if (!client->username.empty())
			(void)client->socket.send(packet);
}

static void clearEngagements(std::uint32_t id, const std::string& username)
{
	for (size_t k = 0; k < invites.size();)
	{
		if (invites[k].fromId == id)
		{
			sf::Packet pkt;
			pkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteCancelled) << username;
			sendTo(invites[k].toId, pkt);
			invites.erase(invites.begin() + static_cast<std::ptrdiff_t>(k));
		}
		else if (invites[k].toId == id)
		{
			sf::Packet pkt;
			pkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteDeclined) << username;
			sendTo(invites[k].fromId, pkt);
			invites.erase(invites.begin() + static_cast<std::ptrdiff_t>(k));
		}
		else
			++k;
	}

	if (Room* room = findRoomInvolving(id))
	{
		std::uint32_t otherId = (room->masterId == id) ? room->guestId : room->masterId;

		sf::Packet pkt;
		pkt << static_cast<std::uint8_t>(Protocol::MessageType::RoomClosed) << (username + " left the room");
		sendTo(otherId, pkt);

		removeRoom(room);
	}
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

	std::uint32_t nextClientId = 1;

	while (true)
	{
		bool ready = selector.wait(sf::seconds(1.f));

		if (ready && selector.isReady(listener))
		{
			auto client = std::make_unique<Client>();
			if (listener.accept(client->socket) == sf::Socket::Status::Done)
			{
				client->id = nextClientId++;
				selector.add(client->socket);
				clients.push_back(std::move(client));
			}
		}

		if (ready)
		{
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
						auto type = static_cast<Protocol::MessageType>(rawType);

						switch (type)
						{
						case Protocol::MessageType::Join:
						{
							std::string username;
							packet >> username;

							bool nameTaken = false;
							for (size_t j = 0; j < clients.size(); ++j)
							{
								if (j != i && clients[j]->username == username)
								{
									nameTaken = true;
									break;
								}
							}

							if (nameTaken)
							{
								std::cout << "[!] Rejected \"" << username << "\" (username already taken)\n";

								sf::Packet rejection;
								rejection << static_cast<std::uint8_t>(Protocol::MessageType::JoinRejected);
								rejection << std::string("Username \"" + username + "\" is already taken");
								(void)client.socket.send(rejection);

								selector.remove(client.socket);
								clients.erase(clients.begin() + static_cast<std::ptrdiff_t>(i));
								removed = true;
							}
							else
							{
								client.username = username;

								std::string address = "unknown";
								if (auto remote = client.socket.getRemoteAddress())
									address = remote->toString();

								std::cout << "[+] \"" << username << "\" connected from " << address << "\n";

								broadcastPlayerList();
							}
							break;
						}

						case Protocol::MessageType::InvitePlayer:
						{
							std::string targetName;
							packet >> targetName;

							Client* target = findClientByUsername(targetName);
							std::string reason;

							if (!target)
								reason = "Player not found";
							else if (target == &client)
								reason = "You can't invite yourself";
							else if (isBusy(client.id))
								reason = "You're already busy";
							else if (isBusy(target->id))
								reason = "\"" + targetName + "\" is busy";

							if (!reason.empty())
							{
								sf::Packet fail;
								fail << static_cast<std::uint8_t>(Protocol::MessageType::InviteFailed) << reason;
								(void)client.socket.send(fail);
							}
							else
							{
								invites.push_back(Invite{ client.id, target->id, sf::Clock() });

								sf::Packet sentPkt;
								sentPkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteSent) << target->username;
								(void)client.socket.send(sentPkt);

								sf::Packet recvPkt;
								recvPkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteReceived) << client.username;
								(void)target->socket.send(recvPkt);
							}
							break;
						}

						case Protocol::MessageType::AcceptInvite:
						{
							auto it = std::find_if(invites.begin(), invites.end(),
								[&](const Invite& inv) { return inv.toId == client.id; });

							if (it != invites.end())
							{
								std::uint32_t masterId = it->fromId;
								invites.erase(it);

								if (Client* master = findClientById(masterId))
								{
									std::uint32_t seed = static_cast<std::uint32_t>(seedGenerator());
									rooms.push_back(Room{ masterId, client.id, seed });

									sf::Packet toMaster;
									toMaster << static_cast<std::uint8_t>(Protocol::MessageType::RoomJoined)
									         << true << client.username << seed;
									(void)master->socket.send(toMaster);

									sf::Packet toGuest;
									toGuest << static_cast<std::uint8_t>(Protocol::MessageType::RoomJoined)
									        << false << master->username << seed;
									(void)client.socket.send(toGuest);
								}
							}
							break;
						}

						case Protocol::MessageType::DeclineInvite:
						{
							auto it = std::find_if(invites.begin(), invites.end(),
								[&](const Invite& inv) { return inv.toId == client.id; });

							if (it != invites.end())
							{
								std::uint32_t fromId = it->fromId;
								invites.erase(it);

								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteDeclined) << client.username;
								sendTo(fromId, pkt);
							}
							break;
						}

						case Protocol::MessageType::CancelInvite:
						{
							auto it = std::find_if(invites.begin(), invites.end(),
								[&](const Invite& inv) { return inv.fromId == client.id; });

							if (it != invites.end())
							{
								std::uint32_t toId = it->toId;
								invites.erase(it);

								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteCancelled) << client.username;
								sendTo(toId, pkt);
							}
							break;
						}

						case Protocol::MessageType::SetSeed:
						{
							std::uint32_t seed = 0;
							packet >> seed;

							if (Room* room = findRoomByMaster(client.id))
							{
								room->seed = seed;

								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::SeedUpdated) << seed;
								sendTo(room->masterId, pkt);
								sendTo(room->guestId, pkt);
							}
							break;
						}

						case Protocol::MessageType::RandomizeSeed:
						{
							if (Room* room = findRoomByMaster(client.id))
							{
								room->seed = static_cast<std::uint32_t>(seedGenerator());

								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::SeedUpdated) << room->seed;
								sendTo(room->masterId, pkt);
								sendTo(room->guestId, pkt);
							}
							break;
						}

						case Protocol::MessageType::StartGame:
						{
							if (Room* room = findRoomByMaster(client.id))
							{
								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::GameStarted) << room->seed;
								sendTo(room->masterId, pkt);
								sendTo(room->guestId, pkt);

								removeRoom(room);
							}
							break;
						}

						case Protocol::MessageType::LeaveRoom:
						{
							if (Room* room = findRoomInvolving(client.id))
							{
								std::uint32_t otherId = (room->masterId == client.id) ? room->guestId : room->masterId;

								sf::Packet pkt;
								pkt << static_cast<std::uint8_t>(Protocol::MessageType::RoomClosed)
								    << (client.username + " left the room");
								sendTo(otherId, pkt);

								removeRoom(room);
							}
							break;
						}

						default:
							break;
						}
					}
					else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error)
					{
						std::cout << "[-] \"" << client.username << "\" disconnected\n";

						clearEngagements(client.id, client.username);

						selector.remove(client.socket);
						clients.erase(clients.begin() + static_cast<std::ptrdiff_t>(i));
						removed = true;

						broadcastPlayerList();
					}
				}

				if (!removed)
					++i;
			}
		}

		for (size_t k = 0; k < invites.size();)
		{
			if (invites[k].clock.getElapsedTime().asSeconds() >= Protocol::InviteTimeoutSeconds)
			{
				std::uint32_t fromId = invites[k].fromId;
				std::uint32_t toId = invites[k].toId;
				invites.erase(invites.begin() + static_cast<std::ptrdiff_t>(k));

				sf::Packet pkt;
				pkt << static_cast<std::uint8_t>(Protocol::MessageType::InviteExpired);
				sendTo(fromId, pkt);
				sendTo(toId, pkt);
			}
			else
				++k;
		}
	}
}
