#pragma once
#include <iostream>
#include <winsock2.h>
#include <algorithm>
#include <mutex>
#include "ClientInfo.h"
#include "Packet.h"
#include "RoomState.h"
#include "RoomManager.h"
#include "Broadcast.h"

void handlePacket(ClientInfo* clientInfo, PacketType type, const char* data, int dataSize) {
    switch (type) {
    case PacketType::CREATEROOM: {
        std::string roomName(data, dataSize);
        Room* room = createRoom(roomName, 4, clientInfo);
        clientInfo->currentRoom = room;

        clientInfo->userId = nextUserId++;
        clientInfo->plantBomb = 0;
        int myId = clientInfo->userId;
        int roomId = room->id;
        
        std::string idPayload;
        idPayload.append(reinterpret_cast<const char*>(&myId), sizeof(int));
        idPayload.append(reinterpret_cast<const char*>(&roomId), sizeof(int));
        sendPacket(clientInfo->socket, PacketType::ASSIGNID, idPayload);

        std::string response = "방 생성 완료! 방 번호: " + std::to_string(room->id);
        sendPacket(clientInfo->socket, PacketType::NOTIFY, response);
        break;
    }
    case PacketType::LISTROOM: {
        listRoom(clientInfo->socket);
        break;
    }
    case PacketType::JOIN: {
        int roomId;
        memcpy(&roomId, data, sizeof(int));
        bool success = joinRoom(roomId, clientInfo);
        if (success) {
            clientInfo->currentRoom = findRoom(roomId);
            clientInfo->userId = nextUserId++;
            clientInfo->plantBomb = 0;
            int myId = clientInfo->userId;
            sendPacket(clientInfo->socket, PacketType::ASSIGNID, std::string(reinterpret_cast<char*>(&myId), sizeof(int)));
        }
        std::string response = success ? "입장 성공" : "입장 실패";
        sendPacket(clientInfo->socket, PacketType::NOTIFY, response);
        break;
    }

    case PacketType::STARTGAME: {
        int roomId;
        memcpy(&roomId, data, sizeof(int));

        Room* room = findRoom(roomId);
        if (room == nullptr || room->players.empty()) {
            break;
        }
        else if (room->players[0] == clientInfo) {
            
            room->state = RoomState::PLAYING;
           
            for (ClientInfo* p : room->players) {
                p->living = true;
            }

            broadcastToRoom(room, PacketType::START, "게임 시작!");
            std::string mapPayload;
            int w = room->map->getWidth();
            int h = room->map->getHeight();
            mapPayload.append(reinterpret_cast<const char*>(&w), sizeof(int));
            mapPayload.append(reinterpret_cast<const char*>(&h), sizeof(int));
            mapPayload.append(reinterpret_cast<const char*>(room->map->getTiles().data()), w * h * sizeof(TileType));

            broadcastToRoom(room, PacketType::MAPINFO, mapPayload);

            int spawnPoints[4][2] = {
                {1, 1},
                {room->map->getWidth() - 2, 1},
                {1, room->map->getHeight() - 2},
                {room->map->getWidth() - 2, room->map->getHeight() - 2}
            };

            for (size_t i = 0; i < room->players.size() && i < 4; i++) {
                ClientInfo* p = room->players[i];
                float spawnX = (float)spawnPoints[i][0] + 0.5f;
                float spawnY = (float)spawnPoints[i][1] + 0.5f;
                p->x = spawnX;
                p->y = spawnY;

                std::string movePayload;
                int pid = p->userId;
                movePayload.append(reinterpret_cast<const char*>(&pid), sizeof(int));
                movePayload.append(reinterpret_cast<const char*>(&spawnX), sizeof(float));
                movePayload.append(reinterpret_cast<const char*>(&spawnY), sizeof(float));

                broadcastToRoom(room, PacketType::MOVE, movePayload);
            }

            break;
        }
        else {
            sendPacket(clientInfo->socket, PacketType::NOTIFY, "권한이 없습니다.");
            break;
        }
    }

    case PacketType::MOVE: {
        float x, y;
        memcpy(&x, data, sizeof(float));
        memcpy(&y, data + sizeof(float), sizeof(float));

        clientInfo->x = x;
        clientInfo->y = y;

        if (clientInfo->currentRoom != nullptr) {
            std::string payload;
            int id = clientInfo->userId;
            payload.append(reinterpret_cast<const char*>(&id), sizeof(int));
            payload.append(reinterpret_cast<const char*>(&x), sizeof(float));
            payload.append(reinterpret_cast<const char*>(&y), sizeof(float));

            broadcastToRoom(clientInfo->currentRoom, PacketType::MOVE, payload, clientInfo->socket);
        }
        break;
    }
    case PacketType::BOMBPLANT: {
        if (clientInfo->plantBomb >= clientInfo->maxBomb) {
            break;
        }

        float x, y;
        memcpy(&x, data, sizeof(float));
        memcpy(&y, data + sizeof(float), sizeof(float));

        Room* room = clientInfo->currentRoom;
        if (room == nullptr) {
            break;
        }

        Bomb bomb;
        bomb.id = nextBombId++;
        bomb.socket = clientInfo->socket;
        bomb.locationX = x;
        bomb.locationY = y;
        room->bombs.push_back(bomb);
        clientInfo->plantBomb++;

        std::string payload;
        payload.append(reinterpret_cast<const char*>(&x), sizeof(float));
        payload.append(reinterpret_cast<const char*>(&y), sizeof(float));
        broadcastToRoom(room, PacketType::BOMBPLANT, payload);

        int bombId = bomb.id;
        std::thread([room, bombId]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            explodeBomb(room, bombId);
        }).detach();

        break;
    }
    default:
        std::cout << "Unknown packet type\n";
        break;
    }
}
void workerThread(HANDLE iocpHandle) {
    while (true) {
        DWORD bytesTransferred;
        ClientInfo* clientInfo = nullptr;
        ULONG_PTR completionKey;

        BOOL result = GetQueuedCompletionStatus(
            iocpHandle,
            &bytesTransferred,
            &completionKey,
            (OVERLAPPED**)&clientInfo, 
            INFINITE
        );

        if (!result || bytesTransferred == 0) {
            std::cout << "Client disconnected!\n";
            leaveRoom(clientInfo);
            std::cout << "result: " << result << " bytesTransferred: " << bytesTransferred << "\n";

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients.erase(std::remove(clients.begin(), clients.end(), clientInfo), clients.end());
            }

            closesocket(clientInfo->socket);
            delete clientInfo;
            continue;
        }

        memcpy(clientInfo->recvBuffer + clientInfo->recvSize, clientInfo->buffer, bytesTransferred);
        clientInfo->recvSize += bytesTransferred;

        while (clientInfo->recvSize >= sizeof(PacketHeader)) {
            PacketHeader header;
            memcpy(&header, clientInfo->recvBuffer, sizeof(PacketHeader));

            if (clientInfo->recvSize < header.size) {
                break;
            }

            const char* payload = clientInfo->recvBuffer + sizeof(PacketHeader);
            int payloadSize = header.size - sizeof(PacketHeader);
            handlePacket(clientInfo, header.type, payload, payloadSize);

            int remaining = clientInfo->recvSize - header.size;
            memmove(clientInfo->recvBuffer, clientInfo->recvBuffer + header.size, remaining);
            clientInfo->recvSize = remaining;
        }

        DWORD flags = 0;
        clientInfo->wsaBuf.buf = clientInfo->buffer;
        clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);
        memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
        WSARecv(clientInfo->socket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
    }
}