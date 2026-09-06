#pragma once
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include "RoomState.h"
#include "Packet.h"
#include "ClientInfo.h"
#include "Broadcast.h"

inline std::vector<Room*> rooms;
inline std::mutex roomsMutex;
inline int nextRoomId = 1;
inline std::atomic<int> nextUserId = 1;
inline std::atomic<int> nextBombId = 1;

inline Room* createRoom(std::string name, int maxPlayers, ClientInfo* player) {
	std::lock_guard<std::mutex> lock(roomsMutex);

	Room* room = new Room();
	room->id = nextRoomId++;
	room->roomName = name;
	room->maxPlayers = maxPlayers;
	room->players.push_back(player);
    room->map = new Map(MAP_WIDTH, MAP_HEIGHT);

	rooms.push_back(room);
	return room;
}

inline void listRoom(SOCKET socket) {
    std::lock_guard<std::mutex> lock(roomsMutex);

    std::string roomList = "";
    for (Room* room : rooms) {
        roomList += std::to_string(room->id) + " :"
            + room->roomName + " : "
            + std::to_string(room->players.size()) + "/"
            + std::to_string(room->maxPlayers) + "\n";
    }
    sendPacket(socket, PacketType::NOTIFY, roomList);
}

inline Room* findRoomUnlocked(int id) {
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i]->id == id) {
            return rooms[i];
        }
    }
    return nullptr;
}

inline Room* findRoom(int id) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    return findRoomUnlocked(id);
}

inline bool joinRoom(int id, ClientInfo* player) {
	std::lock_guard<std::mutex> lock(roomsMutex);

	Room* room = findRoomUnlocked(id);
	if (room == nullptr) {
		return false;
	}
	else {
		if (room->maxPlayers == room->players.size()) {
			return false;
		}
		else {
			room->players.push_back(player);
			return true;
		}
	}

}

inline void deleteRoom(int id) {
    std::lock_guard<std::mutex> lock(roomsMutex);

    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i]->id == id) {
            delete rooms[i]->map;
            rooms.erase(rooms.begin() + i);
            return;
        }
    }
}

inline void leaveRoom(ClientInfo* player) {
    Room* room;
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        room = player->currentRoom;
        if (room == nullptr) {
            return;
        }

        for (int i = 0; i < room->players.size(); i++) {
            if (room->players[i] == player) {
                room->players.erase(room->players.begin() + i);
                break;
            }
        }
        player->currentRoom = nullptr;
    }
    if (room->players.empty()) {
        deleteRoom(room->id);
    }
}

inline void explodeBomb(Room* room, int bombId) {
    std::lock_guard<std::mutex> lock(roomsMutex);

    int aliveCount = 0;
    Bomb bombCopy;
    bool found = false;
    for (int i = 0; i < room->bombs.size(); i++) {
        if (room->bombs[i].id == bombId) {
            bombCopy = room->bombs[i];
            room->bombs.erase(room->bombs.begin() + i);
            found = true;
            break;
        }
    }
    if (!found) return;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (ClientInfo* c : clients) {
            if (c->socket == bombCopy.socket) {
                c->plantBomb--;
                break;
            }
        }
    }
    int tileX, tileY;
    Map::worldToTile(bombCopy.locationX, bombCopy.locationY, tileX, tileY);

    std::vector<std::pair<int, int>> affectedTiles;
    affectedTiles.push_back({ tileX, tileY });

    int dx[4] = { 1, -1, 0, 0 };
    int dy[4] = { 0, 0, 1, -1 };

    for (int dir = 0; dir < 4; dir++) {
        for (int step = 1; step <= bombCopy.BombRange; step++) {
            int nx = tileX + dx[dir] * step;
            int ny = tileY + dy[dir] * step;
            TileType tile = room->map->getTile(nx, ny);
            if (tile == TileType::WALL) break;

            affectedTiles.push_back({ nx, ny });
            if (tile == TileType::BREAKABLE) {
                room->map->destroyBlock(nx, ny);
                break;
            }
        }
    }

    for (ClientInfo* p : room->players) {
        if (!p->living) continue;

        int px, py;
        Map::worldToTile(p->x, p->y, px, py);

        for (auto& tile : affectedTiles) {
            if (tile.first == px && tile.second == py) {
                p->living = false;

                int deadId = p->userId;
                std::string deadPayload(reinterpret_cast<const char*>(&deadId), sizeof(int));
                broadcastToRoom(room, PacketType::PLAYERDIE, deadPayload);
                break;
            }
        }
    }

    for (ClientInfo* p : room->players) {
        if (p->living) {
            aliveCount++;
        }
    }

    if (aliveCount <= 1) {
        room->state = RoomState::END;
        broadcastToRoom(room, PacketType::END, "게임 종료!");
    }

    std::string payload;
    int count = (int)affectedTiles.size();
    payload.append(reinterpret_cast<const char*>(&count), sizeof(int));
    for (auto& tile : affectedTiles) {
        payload.append(reinterpret_cast<const char*>(&tile.first), sizeof(int));
        payload.append(reinterpret_cast<const char*>(&tile.second), sizeof(int));
    }
    broadcastToRoom(room, PacketType::BOOM, payload);
}