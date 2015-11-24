#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <time.h>
using namespace std;


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5000"

int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// initialize random seed: 
	srand(time(NULL));

	// Validate the parameters
	
	string serverAddr = "132.207.29.125";
	cout << "Enter desired a valid ip address (EXAMPLE : 132.207.29.125) \n";
	cin >> serverAddr;
	cout << endl;
	cout << "Server address:" << serverAddr<< endl;
	string port;
	cout << "Enter desired port between 5000 and 5050: ";
	cin >> port;
	cout << endl;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error" << endl;
		system("pause"); return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(serverAddr.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error" << endl;
		WSACleanup();
		system("pause"); return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			cout << "socket failed with error" << WSAGetLastError() << endl;
			WSACleanup();
			system("pause"); return 1;
		}
		

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	char tailleCandidat[1];
	char* candidatCStr;
	// receive the size of candidate string list
	iResult = recv(ConnectSocket, tailleCandidat, 1, 0);
	if (iResult > 0)
	{
		cout << "=========================\nCandidats:" << endl;
		// receive string containing the name of all candidate
		candidatCStr = new char[(int)(tailleCandidat[0])];
		iResult = recv(ConnectSocket, candidatCStr, (int)(tailleCandidat[0]), 0);
		vector<string> listCandidat;
		string tempCandidat = "";
		for (int i = 0; i < (int)(tailleCandidat[0]); ++i)
		{
			if (candidatCStr[i] == ';' && tempCandidat.size() != 0) 
			{
				//jump to next candidat
				cout << tempCandidat << endl;
				listCandidat.push_back(tempCandidat);
				tempCandidat = ""; // reset buffer
			} 
			else if (candidatCStr[i] != ';')
			{
				tempCandidat += candidatCStr[i];
			}
		}

		// Vote random
		int choice = rand() % listCandidat.size();
		cout << "Voted for " + listCandidat.at(choice) << endl;
		// Send vote back...
		//...
		char choiceChar = (char)choice;
		// Send an initial buffer
		iResult = send(ConnectSocket, &choiceChar, 1, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			system("pause"); return 1;
		}

		printf("Choice Sent with %ld bits\n", iResult);
	

	}
	else {
		printf("Erreur de reception : %d\n", WSAGetLastError());
	}


	freeaddrinfo(result);


	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!" << endl;
		WSACleanup();
		system("pause"); return 1;
	}
	

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		system("pause"); return 1;
	}


	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	system("pause"); return 0;
}