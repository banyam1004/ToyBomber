#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include "ClientInfo.h"
#include "WorkerThread.h"
#include "Broadcast.h"
#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed\n";

		return 1;
	}

	HANDLE iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocpHandle == NULL) {
		std::cerr << "IOCP creation failed\n";

		return 1;
	}
	std::cout << "IOCP created!\n";

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";

		return 1;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "bind() failed\n";

		return 1;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "listen() failed\n";

		return 1;
	}
	
	std::cout << "Server Start!\n";

	std::vector<std::thread> threads;
	for (int i = 0; i < 4; i++) {
		threads.emplace_back(workerThread, iocpHandle);
	}

	while (true) {
		SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET) break;
		std::cout << "Client connected\n";

		ClientInfo* clientInfo = new ClientInfo();
        memset(&clientInfo->overlapped, 0, sizeof(OVERLAPPED));
		clientInfo->socket = clientSocket;
		clientInfo->wsaBuf.buf = clientInfo->buffer;
		clientInfo->wsaBuf.len = sizeof(clientInfo->buffer);

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientInfo);
        }

		CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, (ULONG_PTR)clientInfo, 0);

		DWORD flags = 0;
		WSARecv(clientSocket, &clientInfo->wsaBuf, 1, nullptr, &flags, &clientInfo->overlapped, nullptr);
	}

	CloseHandle(iocpHandle);
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}