#pragma once
#include <winsock2.h>

struct Bomb {
    int id = 0;
	SOCKET socket;
	float locationX = 0.0f;
	float locationY = 0.0f;
	float BombTimer = 3.0f;
	int BombRange = 3;
};