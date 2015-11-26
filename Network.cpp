#include "Network.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <string>

#include <windows.h> // Includes for TCP networking.
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

Network::Network() : ListenSocket(INVALID_SOCKET)
{
}

bool Network::CreateHost() 
{
	WSADATA wsaData;
	int iResult;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock.
	if(iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); // Resolve the server address and port.
	if(iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return false;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol); // Create a SOCKET for connecting to server.
	if(ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // Setup the TCP listening socket.
	if(iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if(iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	std::cout << "Waiting for players..." << std::endl;
	return true;

	// closesocket(ListenSocket); // No longer need server socket.
}

void Network::ListenConnection()
{
	while(true)
	{
		SOCKET* ClientSocket = new SOCKET(INVALID_SOCKET);

		*ClientSocket = accept(ListenSocket, NULL, NULL); // Wait for client socket.

		if(*ClientSocket == INVALID_SOCKET) // Error initialising the socket.
			printf("ListenConnection failed with error: %d\n", WSAGetLastError());
		else
		{
			Player* player = new Player(ClientSocket);
			players.push_back(player); // Create new player upon successful connection.
			std::cout << "New player connected ID(" << player->ID << ")." << std::endl;

			std::thread message_thread(&Network::ListenMessage, this, player); // Start new thread for communication.
		}
	}
}

void Network::ListenMessage(Player* player)
{
	std::cout << "Starting ListenMessage for ID(" << player->ID << ")." << std::endl;

	int iResult = 0, iSendResult = 0; // Variables for testing message receiving and sending.
	char recvbuf[DEFAULT_BUFLEN]; // Buffer for messages.
	int recvbuflen = DEFAULT_BUFLEN; // Length of buffer.

	// Condition needs to be remade later!
	while(true) // Receive messages until the peer shuts down the connection.
	{
		iResult = recv(*player->GetSocket(), recvbuf, recvbuflen, 0); // Receive message from client.

		if(iResult > 0) // If receiving message was successfull.
		{
			//printf("Bytes received: %d\n", iResult); // How many bytes received from client.

			std::string message(recvbuf); // Make std::string out of received message.
			int size = message.find('\0'); // Find first newline to determine real size.
			message.resize(size);
			std::cout << "(" << player->ID << "): " << message << std::endl; // Print received message.

			//iSendResult = send(*player->GetSocket(), recvbuf, iResult, 0); // Echo the buffer back to the sender.
			//if(iSendResult == SOCKET_ERROR)
				//std::cout << "SEND failed to (" << player->ID << "): " << WSAGetLastError() << std::endl;

			//if(iResult == iSendResult) // See if byte amounts match for received and sent.
				//std::cout << "Connection established with ID(" << player->ID << ")." << std::endl;
				//; // Temporary.
			//else
				//std::cout << "Possible connection problems with ID(" << player->ID << ")." << std::endl;
		}
		else if(iResult == 0)
		{
			std::cout << "Connection closing for ID(" << player->ID << ")." << std::endl;
			// Need to add cleaning later.
		}
		else 
		{
			std::cout << "RECEIVE failed from ID(" << player->ID << ")." << std::endl;
			//closesocket(ClientSocket);
			//WSACleanup();
		}
	}
}

void Network::Clean()
{
	// Possible functions needed to clean network functionality.
	WSACleanup();
}

void Network::ClearBuffer(char(&buffer)[10])
{
	for(int i = 0; i < DEFAULT_BUFLEN; i++)
		buffer[i] = '\0';
}

int Network::ConnectedPlayers()
{
	return players.size();
}