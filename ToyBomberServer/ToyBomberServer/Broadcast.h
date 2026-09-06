#pragma once
#include <winsock2.h>
#include <vector>
#include <mutex>
#include "ClientInfo.h"
#include "Packet.h"
#include "RoomState.h"

inline std::vector<ClientInfo*> clients;
inline std::mutex clientsMutex;

inline void broadcast(PacketType type, const std::string& message, SOCKET sender = INVALID_SOCKET) {
	std::lock_guard<std::mutex> lock(clientsMutex);
	for (ClientInfo* client : clients) {
		if (client->socket != sender) {
			sendPacket(client->socket, type, message);
		}
	}
}

inline void broadcastToRoom(Room* room, PacketType type, const std::string& message, SOCKET sender = INVALID_SOCKET) {
    for (ClientInfo* client : room->players) {
        if (client->socket != sender) {
            sendPacket(client->socket, type, message);
        }
    }
}