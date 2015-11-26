#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <list>

// Need to link with Ws2_32.lib
/* winemaker: Added -lWs2_32 to the libraries */
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

std::list<SOCKET *> activeSocket;

std::mutex socketlistmtx;
void AddActiveSocket(SOCKET *s) {
	socketlistmtx.lock();
	activeSocket.push_back(s);
	socketlistmtx.unlock();
}

void DeleteSocket(SOCKET *s) {
	socketlistmtx.lock();
	activeSocket.remove(s);
	socketlistmtx.unlock();
}

//std::mutex sendmtx;
void SendAll(char *str) {
	socketlistmtx.lock();
	for(auto &s : activeSocket) {
		send(*s, str, strlen(str), 0);
	}
	socketlistmtx.unlock();
}

void NewClient(SOCKET ClientSocket) {
	AddActiveSocket(&ClientSocket);
	int iResult;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	static int thcount = 1;
	int thisthread = thcount;
	thcount++;
	printf("thread %d started\n", thisthread);
	// Receive until the peer shuts down the connection
	while(1) {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if(iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			SendAll(recvbuf);
		}
		else if(iResult == 0) {
			printf("Connection closing... th=%d\n", thisthread);
			DeleteSocket(&ClientSocket);
			closesocket(ClientSocket);
			return;
		}
		else  {
			printf("recv failed with error: %d\n", WSAGetLastError());
			DeleteSocket(&ClientSocket);
			closesocket(ClientSocket);
			return;
		}
	}

	DeleteSocket(&ClientSocket);
	printf("thread completed\n");
}

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if(iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if(iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	while(1) {

		iResult = listen(ListenSocket, SOMAXCONN);
		if(iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}


		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if(ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		std::thread th(NewClient, ClientSocket);
		th.detach();
	}
	// No longer need server socket
	// closesocket(ListenSocket);


	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if(iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}
