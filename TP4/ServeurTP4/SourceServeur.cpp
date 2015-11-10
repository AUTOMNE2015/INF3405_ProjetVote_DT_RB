#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>
#include <strstream>
#include <vector>
#include <string>
#include <map>
#include <ctime>
#include <fstream>

using namespace std;
#define DEFAULT_PORT "5000"



#pragma comment(lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS = 0

SOCKET CreateSocket(string Socket);
DWORD WINAPI CheckClock(void* setOfListenSockets);
char* GetCandidateString(map<string, int>* m);
bool GetCandidates(string nomFichier, map<string, int>* m);
DWORD WINAPI ProcessVoter(void* param);


struct processParameters
{
	SOCKET socket;
	char* candidateNames;
	int sizeOfCandidateNames;
};

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
	fd_set* setOfListenSockets = new fd_set();

	*setOfListenSockets = { 50, *listenSockets };
	std::cout << "Listen success, waiting for connection..." << endl;

	


	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;

	map<string, int>* candidates = new map<string,int>(); 
	GetCandidates("fichier.txt", candidates); 
	string tmpCandidateString = "";
	for (map<string, int>::iterator it = candidates->begin(); it != candidates->end(); it++)
		tmpCandidateString += it->first + ";";

	char* candidateCstr; 
	candidateCstr = _strdup(tmpCandidateString.c_str()); // duplique le string vers un c string sur le heap


	processParameters* param = new processParameters();
	DWORD ThreadClockID;
	CreateThread(0, 0, CheckClock, (void*)(setOfListenSockets), 0, &ThreadClockID);
	// Accept a client socket
	while (1)
	{
		//int result = select(0, setOfListenSockets, NULL,NULL,NULL);
		//if (result != -1)
			//std::cout << "result" << result << endl;
		ClientSocket = accept(listenSockets[0], NULL, NULL);
		
		*param = { ClientSocket, candidateCstr, strlen(candidateCstr) };
		if (ClientSocket == INVALID_SOCKET) {
			std::cout << "accept failed: " << WSAGetLastError() << endl;
			//closesocket(listenSockets[0]);
			WSACleanup();
			while (1);
			return 1;
			}
			DWORD nThreadID;
			CreateThread(0, 0, ProcessVoter, (void*)param, 0, &nThreadID);
			//ProcessVoter((void*)param);
	//std::cout << "accept success!" << endl;
	}

	delete candidateCstr;
	delete candidates;
	delete setOfListenSockets;
	

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
	iResult = getaddrinfo(NULL, port.c_str(), &hints, &result); //"132.207.29.125"
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

bool GetCandidates(string nomFichier, map<string,int>* m)
{
	string result = "";
	string tmp;
	ifstream file;
	file.open(nomFichier);
	if (!file.good())
		return false;
	while (!file.eof())
	{
		std::getline(file, tmp, ';');
		m->insert({ tmp, 0 });
	}
	return true;
}

DWORD WINAPI CheckClock(void* setOfListenSockets)
{
	double startTime = clock() / (double)CLOCKS_PER_SEC;
	const double endTime = 120;
	double currentTime = clock() / (double)CLOCKS_PER_SEC - startTime;
	while (endTime > currentTime)
	{
		currentTime = clock() / (double)CLOCKS_PER_SEC - startTime;
	}
	fd_set* tmp;
	tmp = (fd_set*)setOfListenSockets;
	for (int i = 0; i < 50; ++i)
		closesocket(tmp->fd_array[i]);

	return 0;
}

DWORD WINAPI ProcessVoter(void* param)
{
	//-----------------------------
	// Envoyer le mot au serveur
	processParameters* castedParameters = (processParameters*)param;

	// envoie de la taille de la liste des candidats
	char nb = castedParameters->sizeOfCandidateNames;
	int iResult1 = send(castedParameters->socket, &nb, 1, 0);
	if (iResult1 == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(castedParameters->socket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	//envoie du char* contenant le noms des tous les candidats
	int iResult2 = send(castedParameters->socket, castedParameters->candidateNames, castedParameters->sizeOfCandidateNames, 0);
	if (iResult2 == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(castedParameters->socket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	printf("Nombre d'octets envoyes : %ld\n", iResult1);
	return 0;
}
