#pragma once
#include <cstdint>
#include <string>

namespace Protocol
{
	const uint16_t PORT = 55001;
	const std::string ServerAddress = "127.0.0.1";
	const float InviteTimeoutSeconds = 30.f;

	enum class MessageType : std::uint8_t
	{
		Join,            
		PlayerList,     
		JoinRejected,  

		InvitePlayer, 
		InviteFailed,  
		InviteSent,     
		InviteReceived, 
		InviteCancelled, 
		InviteDeclined, 
		InviteExpired, 

		AcceptInvite,  
		DeclineInvite,  
		CancelInvite,  

		RoomJoined,
		SetSeed,
		RandomizeSeed,
		SeedUpdated,
		StartGame,
		GameStarted,
		LeaveRoom,
		RoomClosed,

		PlayerFinished,  
		OpponentFinished,
	};
}
