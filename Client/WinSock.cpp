#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
/* winemaker: Added -lWs2_32 to the libraries */
/* winemaker: Added -lMswsock to the libraries */
/* winemaker: Added -lAdvApi32 to the libraries */

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "Advapi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define SERVER "127.0.0.1"

void ReceiveThread(SOCKET &ConnectSocket) {
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	while(1) {
		memset(recvbuf, 0, sizeof(recvbuf));
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if(iResult > 0)
			printf("%s\n", recvbuf);
		else if(iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());
	}
}

int __cdecl main(void)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Validate the parameters
	/*if(argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}*/

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if(iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for(ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
							   ptr->ai_protocol);
		if(ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if(iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if(ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	std::thread th(ReceiveThread, ConnectSocket);
	th.detach();

	while(1) {
		// Send an initial buffer
		gets(sendbuf);
		iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
		if(iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		printf("Bytes Sent: %ld\n", iResult);
	}
	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
