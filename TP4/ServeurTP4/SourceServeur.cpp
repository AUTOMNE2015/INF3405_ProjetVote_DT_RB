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
#include <mutex>

#define DEFAULT_PORT "5000"
#define _WINSOCK_DEPRECATED_NO_WARNINGS = 0


#pragma comment(lib, "Ws2_32.lib")

// forward declarations
SOCKET CreateSocket(std::string Socket);
DWORD WINAPI CheckClock(void* setOfListenSockets);
char* GetCandidatestring(std::map<std::string, int>* m);
bool GetCandidates(std::string nomFichier, std::map<std::string, int>* m);
DWORD WINAPI ProcessVoter(void* param);
std::string GetCurrentDateTime();
// global variables
std::map<std::string, int>* candidates;
std::mutex vote_mutex;

bool ended = false;
const double ENDTIME = 60.0; //Election closes in 60 seconds

// structure for passing parameters to threads
struct processParameters
{
	SOCKET socket;
	char* candidateNames;
	int sizeOfCandidateNames;
	std::string ipClient;
};

int main(void)
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed" << std::endl;
		while (1);
		return 1;
	}
	//std::cout << "WSAStartup success!" << std::endl;

	// Open 50 sockets
	SOCKET listenSockets[50];
	for (int i = 0; i < 50; ++i)
	{
		listenSockets[i] = CreateSocket(std::to_string(5000 + i));

		if (listen(listenSockets[i], SOMAXCONN) == SOCKET_ERROR) {
			std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listenSockets[i]);
			WSACleanup();
			while (1);
			return 1;
		}
	}

	// initialize fd_set for sockets
	fd_set* setOfListenSockets = new fd_set();
	*setOfListenSockets = { 50, *listenSockets };

	std::cout << "====================================================" << std::endl;
	std::cout << "Initialization succeeded, waiting for connection..." << std::endl;

	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;

	// Map candidats
	candidates = new std::map<std::string, int>();
	GetCandidates("fichier.txt", candidates); 

	// Creer un c-string avec tous les candidats
	std::string tmpCandidatestring = "";
	for (std::map<std::string, int>::iterator it = candidates->begin(); it != candidates->end(); it++)
		tmpCandidatestring += it->first + ";";

	char* candidateCstr; 
	candidateCstr = _strdup(tmpCandidatestring.c_str()); // duplique le std::string vers un c-string sur le heap

	// Create timer
	processParameters* param = new processParameters();
	DWORD ThreadClockID;
	CreateThread(0, 0, CheckClock, (void*)(setOfListenSockets), 0, &ThreadClockID);
	
	// Program loop
	while (!ended)
	{
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 10;

		// loop to listen and accept connections
		fd_set tempSet;
		for (int i = 0; i < 50; ++i)
		{
			FD_ZERO(&tempSet); // clear the set 
			FD_SET(listenSockets[i], &tempSet); // add our file descriptor to the set
			int r = select(0, &tempSet, 0, 0, &timeout);
			if (r > 0)
			{
				// accept connection
				sockaddr_in sinRemote;
				int nAddrSize = sizeof(sinRemote);
				char ipstr[INET_ADDRSTRLEN + 1];
				int port = i +5000;

				ClientSocket = accept(listenSockets[i], (sockaddr*)&sinRemote, &nAddrSize);

				
				std::string ipClient = inet_ntop(AF_INET, &sinRemote.sin_addr, ipstr, sizeof(sockaddr_in));
				ipClient += ":" + std::to_string(5000 + i);
				////

				*param = { ClientSocket, candidateCstr, strlen(candidateCstr), ipClient };
				if (ClientSocket == INVALID_SOCKET) {
					std::cout << "accept failed: " << WSAGetLastError() << std::endl;
					WSACleanup();
					ended = true;
				}
				
				// Create a thread to process the connection
				DWORD nThreadID;
				CreateThread(0, 0, ProcessVoter, (void*)param, 0, &nThreadID);
			}
		}
		//std::cout << "accept success!" << std::endl;
	}

	// close sockets
	for (int i = 0; i < 50; ++i)
		closesocket(setOfListenSockets->fd_array[i]);

	// Get winning candidate after timeout
	int max = candidates->begin()->second;
	std::string winner = candidates->begin()->first;
	for (auto it = candidates->begin(); (it != candidates->end()); it++)
	{
		if (it->second > max)
		{
			max = it->second;
			winner = it->first;
		}
	}
	std::cout << "End of elections. Winner of election is " << winner << "!!!!!!!!" << std::endl;

	std::cout << "Detailed results:" << std::endl;
	for (auto it = candidates->begin(); (it != candidates->end()); it++)
	{
		std::cout << it->first << " : " << std::to_string(it->second) << " votes" << std::endl;
	}

	// clean up
	delete candidateCstr;
	delete candidates;
	delete setOfListenSockets;

	system("pause");
	return 0;
}

//Function to create socket on a specified port
SOCKET CreateSocket(std::string port)
{
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int iResult;
	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed" << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	// initialize Socket
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		std::cout << "Error at socket() function" << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Bind socket to ip address :
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "bind failed with error" << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	// no need of result anymore
	freeaddrinfo(result);

	return ListenSocket;
}

// function to read candiates from file and populate candidate map
bool GetCandidates(std::string nomFichier, std::map<std::string,int>* m)
{
	std::string tmp;
	std::ifstream file;
	file.open(nomFichier);
	if (!file.good())
	{
		std::cout << "File not found" << std::endl;
		return false;
	}
	while (!file.eof())
	{
		std::getline(file, tmp, ';');
		if (tmp.size() != 0) { m->insert({ tmp, 0 }); }
	}
	return true;
}

// function to check if time is expired
DWORD WINAPI CheckClock(void* setOfListenSockets)
{
	double startTime = clock() / (double)CLOCKS_PER_SEC;
	double currentTime = clock() / (double)CLOCKS_PER_SEC - startTime;
	while (ENDTIME > currentTime)
	{
		currentTime = clock() / (double)CLOCKS_PER_SEC - startTime;
	}

	ended = true;
	return 0;
}

// function to allow client to vote for a candidate
DWORD WINAPI ProcessVoter(void* param)
{
	processParameters* castedParameters = (processParameters*)param;

	// send size of the string containing all candidates that will be sent in the next send
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

	//Send candidate name in char*
	int iResult2 = send(castedParameters->socket, castedParameters->candidateNames, castedParameters->sizeOfCandidateNames, 0);
	if (iResult2 == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(castedParameters->socket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	// receive vote and count vote	
	char choiceChar;
	int iResult3 = recv(castedParameters->socket, &choiceChar, 1, 0);
	std::cout << "Received vote : " << ((int)choiceChar) << std::endl;
	auto it = candidates->begin();
	for (int i = 0; (it != candidates->end()) && (i < (int)choiceChar); i++)
	{
		it++;
	}

	std::lock_guard<std::mutex> guard(vote_mutex); // take ownership of mutex
	it->second++;
	// write to journal
	std::ofstream outfile;
	outfile.open("journalDeVoteurs.txt", std::ios_base::app);
	outfile << castedParameters->ipClient << " " << GetCurrentDateTime();

	//the mutex is automatically released when the function ends
	return 0;
}

// returns current datetime in a string
std::string GetCurrentDateTime()
{
	time_t rawCurrentTime;
	struct tm timeinfo;
	time(&rawCurrentTime);
	localtime_s(&timeinfo, &rawCurrentTime);
	char convertedTime[256];
	asctime_s(convertedTime, 256, &timeinfo);
	return convertedTime;
}
