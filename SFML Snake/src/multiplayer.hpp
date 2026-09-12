#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include <string>
#include <vector>

enum class LobbyStatus
{
	Idle,
	InviteSent,     
	InviteReceived, 
	InRoom,         
};

class MultiplayerClient
{
	sf::TcpSocket socket;
	bool connected = false;
	std::vector<std::string> players;
	std::string lastError;

	LobbyStatus status = LobbyStatus::Idle;
	std::string invitePeer;
	sf::Clock inviteClock;

	bool roomIsMaster = false;
	std::string opponentUsername;
	std::uint32_t roomSeed = 0;

	bool gameStartedPending = false;
	std::uint32_t gameStartedSeed = 0;

	std::string notice;

	void resetSocket();
	void applyPlayerList(sf::Packet& packet);
	void resetLobbyState();

public:
	bool connect(const std::string& serverAddress, uint16_t port, const std::string& username);
	void disconnect();
	void update();
	bool isConnected() const;
	const std::vector<std::string>& getPlayers() const;
	const std::string& getLastError() const;

	LobbyStatus getStatus() const;
	void invitePlayer(const std::string& username);
	void acceptInvite();
	void declineInvite();
	void cancelInvite();
	const std::string& getInvitePeer() const;
	float getInviteRemainingSeconds() const;

	bool isRoomMaster() const;
	const std::string& getOpponentUsername() const;
	std::uint32_t getRoomSeed() const;
	void setSeed(std::uint32_t seed);
	void randomizeSeed();
	void startGame();
	void leaveRoom();
	bool consumeGameStarted(std::uint32_t& seedOut);

	bool hasNotice() const;
	std::string takeNotice();
};
