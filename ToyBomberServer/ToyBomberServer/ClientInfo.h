#pragma once
#include <winsock2.h>

struct Room;

struct ClientInfo {
	OVERLAPPED overlapped;
	SOCKET socket;
	WSABUF wsaBuf;
	char buffer[1024];
	char name[50];
	char recvBuffer[4096];
	int recvSize = 0;

	int userId = 0;
	bool living = false;
	float x = 0.0f;
	float y = 0.0f;
	int maxBomb = 1;
	int plantBomb = 0;

    Room* currentRoom = nullptr;
};