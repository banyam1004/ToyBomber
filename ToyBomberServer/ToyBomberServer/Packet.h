#pragma once
#include <winsock2.h>
#include <vector>
#include <string>
#include <cstdint>

enum class PacketType : uint16_t {
	MOVE = 1,
	BOMBPLANT = 2,
	BOOM = 3,
	PLAYERDIE = 4,
	START = 5,      //실제 게임 시작
	END = 6,
	JOIN = 7,       // 방 입장
	NOTIFY = 8,
    CREATEROOM = 9,
    LISTROOM = 10,
    STARTGAME = 11, // 방장의 게임 시작
    MAPINFO = 12,
    ASSIGNID = 13
};

#pragma pack(push, 1)
struct PacketHeader {
	PacketType type;
	uint16_t size;
};
#pragma pack(pop)

inline void sendPacket(SOCKET socket, PacketType type, const std::string& data) {
	PacketHeader header;
	header.type = type;
	header.size = sizeof(PacketHeader) + data.size();

	std::vector<char> packet(header.size);
	memcpy(packet.data(), &header, sizeof(PacketHeader));
	memcpy(packet.data() + sizeof(PacketHeader), data.c_str(), data.size());

	send(socket, packet.data(), header.size, 0);
}