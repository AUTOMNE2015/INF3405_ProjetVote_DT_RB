#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>
#include <strstream>
#include <vector>
#include <string>

using namespace std;
#define DEFAULT_PORT "5000"



#pragma comment(lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS = 0

SOCKET CreateSocket(string Socket);

int main(void)
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed" << endl;
		while (1);
		return 1;
	}
	std::cout << "WSAStartup success!" << endl;

	SOCKET listenSockets[50];
	
	for (int i = 0; i < 50; ++i)
	{
		listenSockets[i] = CreateSocket(to_string(5000 + i));

		if (listen(listenSockets[i], SOMAXCONN) == SOCKET_ERROR) {
			std::cout << "Listen failed with error: " << WSAGetLastError() << endl;
			closesocket(listenSockets[i]);
			WSACleanup();
			while (1);
			return 1;
		}
	}
	fd_set setOfListenSockets = { 50, *listenSockets };
	std::cout << "Listen success, waiting for connection..." << endl;

	


	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	while (1)
	{
		int result = select(0, &setOfListenSockets, NULL,NULL,NULL);
		if (result != -1)
			std::cout << "result" << result << endl;
			//ClientSocket = accept(listenSockets[0], NULL, NULL);
		/*
			if (ClientSocket == INVALID_SOCKET) {
				std::cout << "accept failed: " << WSAGetLastError() << endl;
				//closesocket(listenSockets[0]);
				WSACleanup();
				while (1);
				return 1;
			}*/
	//std::cout << "accept success!" << endl;
	}

	

	return 0;
}

SOCKET CreateSocket(string port)
{
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int iResult;
	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, port.c_str(), &hints, &result); //"132.207.29.124"
	if (iResult != 0) {
		cout << "getaddrinfo failed" << endl;
		WSACleanup();
		return INVALID_SOCKET;
	}
	cout << "getaddrinfo success!" << endl;


	// initialize Socket
	SOCKET ListenSocket = INVALID_SOCKET;

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		cout << "Error at socket() function" << endl;
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}
	cout << "Success at socket() function!" << endl;

	// Bind socket to ip address :
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error" << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	// no need of result anymore
	freeaddrinfo(result);

	cout << "bind success!" << endl;
	return ListenSocket;
}

