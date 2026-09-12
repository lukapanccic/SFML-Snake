#include "multiplayer.hpp"
#include "Protocol.hpp"

bool MultiplayerClient::connect(const std::string& serverAddress, uint16_t port, const std::string& username)
{
	resetSocket();
	connected = false;
	players.clear();
	resetLobbyState();

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

void MultiplayerClient::resetLobbyState()
{
	status = LobbyStatus::Idle;
	invitePeer.clear();
	roomIsMaster = false;
	opponentUsername.clear();
	roomSeed = 0;
	gameStartedPending = false;
	gameStartedSeed = 0;
	opponentResultPending = false;
	opponentResultScore = 0;
	opponentLeftPending = false;
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
	resetLobbyState();
	notice.clear();
}

void MultiplayerClient::update()
{
	if (!connected)
		return;

	while (true)
	{
		sf::Packet packet;
		sf::Socket::Status status_ = socket.receive(packet);

		if (status_ == sf::Socket::Status::NotReady)
			break;

		if (status_ != sf::Socket::Status::Done)
		{
			resetSocket();
			connected = false;
			players.clear();
			resetLobbyState();
			break;
		}

		std::uint8_t rawType = 0;
		packet >> rawType;
		auto type = static_cast<Protocol::MessageType>(rawType);

		switch (type)
		{
		case Protocol::MessageType::PlayerList:
			applyPlayerList(packet);
			break;

		case Protocol::MessageType::InviteFailed:
		{
			std::string reason;
			packet >> reason;
			notice = reason;
			break;
		}

		case Protocol::MessageType::InviteSent:
		{
			std::string toUsername;
			packet >> toUsername;
			status = LobbyStatus::InviteSent;
			invitePeer = toUsername;
			inviteClock.restart();
			break;
		}

		case Protocol::MessageType::InviteReceived:
		{
			std::string fromUsername;
			packet >> fromUsername;
			status = LobbyStatus::InviteReceived;
			invitePeer = fromUsername;
			inviteClock.restart();
			break;
		}

		case Protocol::MessageType::InviteCancelled:
		{
			std::string byUsername;
			packet >> byUsername;
			notice = byUsername + " cancelled the invite";
			status = LobbyStatus::Idle;
			invitePeer.clear();
			break;
		}

		case Protocol::MessageType::InviteDeclined:
		{
			std::string byUsername;
			packet >> byUsername;
			notice = byUsername + " declined the invite";
			status = LobbyStatus::Idle;
			invitePeer.clear();
			break;
		}

		case Protocol::MessageType::InviteExpired:
			notice = "The invite expired";
			status = LobbyStatus::Idle;
			invitePeer.clear();
			break;

		case Protocol::MessageType::RoomJoined:
		{
			bool isMaster = false;
			std::string opponent;
			std::uint32_t seed = 0;
			packet >> isMaster >> opponent >> seed;

			status = LobbyStatus::InRoom;
			roomIsMaster = isMaster;
			opponentUsername = opponent;
			roomSeed = seed;
			invitePeer.clear();
			break;
		}

		case Protocol::MessageType::SeedUpdated:
			packet >> roomSeed;
			break;

		case Protocol::MessageType::GameStarted:
			packet >> gameStartedSeed;
			gameStartedPending = true;
			break;

		case Protocol::MessageType::RoomClosed:
		{
			std::string reason;
			packet >> reason;
			notice = reason;
			status = LobbyStatus::Idle;
			opponentUsername.clear();
			opponentLeftPending = true;
			break;
		}

		case Protocol::MessageType::OpponentFinished:
			packet >> opponentResultScore;
			opponentResultPending = true;
			break;

		default:
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

LobbyStatus MultiplayerClient::getStatus() const
{
	return status;
}

void MultiplayerClient::invitePlayer(const std::string& username)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::InvitePlayer) << username;
	(void)socket.send(packet);
}

void MultiplayerClient::acceptInvite()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::AcceptInvite);
	(void)socket.send(packet);
}

void MultiplayerClient::declineInvite()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::DeclineInvite);
	(void)socket.send(packet);

	status = LobbyStatus::Idle;
	invitePeer.clear();
}

void MultiplayerClient::cancelInvite()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::CancelInvite);
	(void)socket.send(packet);

	status = LobbyStatus::Idle;
	invitePeer.clear();
}

const std::string& MultiplayerClient::getInvitePeer() const
{
	return invitePeer;
}

float MultiplayerClient::getInviteRemainingSeconds() const
{
	float remaining = Protocol::InviteTimeoutSeconds - inviteClock.getElapsedTime().asSeconds();
	return remaining > 0.f ? remaining : 0.f;
}

bool MultiplayerClient::isRoomMaster() const
{
	return roomIsMaster;
}

const std::string& MultiplayerClient::getOpponentUsername() const
{
	return opponentUsername;
}

std::uint32_t MultiplayerClient::getRoomSeed() const
{
	return roomSeed;
}

void MultiplayerClient::setSeed(std::uint32_t seed)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::SetSeed) << seed;
	(void)socket.send(packet);
}

void MultiplayerClient::randomizeSeed()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::RandomizeSeed);
	(void)socket.send(packet);
}

void MultiplayerClient::startGame()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::StartGame);
	(void)socket.send(packet);
}

void MultiplayerClient::leaveRoom()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::LeaveRoom);
	(void)socket.send(packet);

	resetLobbyState();
}

bool MultiplayerClient::consumeGameStarted(std::uint32_t& seedOut)
{
	if (!gameStartedPending)
		return false;

	seedOut = gameStartedSeed;
	gameStartedPending = false;
	return true;
}

void MultiplayerClient::reportFinished(std::uint32_t score)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(Protocol::MessageType::PlayerFinished) << score;
	(void)socket.send(packet);
}

bool MultiplayerClient::consumeOpponentResult(std::uint32_t& scoreOut)
{
	if (!opponentResultPending)
		return false;

	scoreOut = opponentResultScore;
	opponentResultPending = false;
	return true;
}

bool MultiplayerClient::consumeOpponentLeft()
{
	if (!opponentLeftPending)
		return false;

	opponentLeftPending = false;
	return true;
}

bool MultiplayerClient::hasNotice() const
{
	return !notice.empty();
}

std::string MultiplayerClient::takeNotice()
{
	std::string result = std::move(notice);
	notice.clear();
	return result;
}
