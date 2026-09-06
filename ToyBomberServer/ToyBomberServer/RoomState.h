#pragma once
#include <vector>
#include <string>
#include "ClientInfo.h"
#include "Bomb.h"
#include "Map.h"

enum class RoomState {
	WAITING,
	PLAYING,
	END
};

struct Room {
	int id;
	std::string roomName;
	RoomState state = RoomState::WAITING;
	std::vector<ClientInfo*> players;
	std::vector<Bomb> bombs;
    Map* map = nullptr;
	int maxPlayers = 4;
};