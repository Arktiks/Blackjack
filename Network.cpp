#include "Network.h"
#include <iostream>
#include <thread>
#include <string>
#include <assert.h>

#include <windows.h> // Includes for TCP networking.
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define LOCK Network::lock
#define PLAYERS Network::players

std::vector<Player*> Network::players;
SOCKET Network::ListenSocket;
std::mutex* Network::lock = nullptr;

Network::Network()
{
	if(lock == nullptr)
	{
		lock = new std::mutex;
		assert(lock);
	}
}

Network::Network(Network& network)
{
	lock = network.lock;
	assert(lock);
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

	std::cout << "Host succesfully initialised." << std::endl;
	return true;
}

void Network::ListenConnection()
{
	while(true)
	{
		SOCKET* ClientSocket = new SOCKET(INVALID_SOCKET);
		*ClientSocket = accept(Network::ListenSocket, NULL, NULL); // Blocking call waiting for client socket.

		if(*ClientSocket == INVALID_SOCKET) // Error initialising the socket.
		{
			printf("ListenConnection failed with error: %d\n", WSAGetLastError());
		}
		else // Initialise new player with created socket.
		{
			Player* player = new Player(ClientSocket);
			LOCK->lock();
			PLAYERS.push_back(player); // Create new player upon successful connection.
			LOCK->unlock();
			std::cout << "New player connected ID(" << player->ID << ")." << std::endl;

			std::thread message_thread(&Network::ListenMessage, this, player); // Start new thread for communication.
			message_thread.detach();
		}
	}
}

void Network::ListenMessage(Player* player)
{
	std::cout << "Starting ListenMessage for ID(" << player->ID << ")." << std::endl;

	SOCKET* socket = player->GetSocket(); // Handle to players socket.

	int receive_error = 0; // Keep track of networking errors.

	while(true) // Receive messages until the peer shuts down the connection.
	{
		char* recvbuf = new char[DEFAULT_BUFLEN](); // Buffer for messages.

		int iResult = recv(*socket, recvbuf, DEFAULT_BUFLEN, 0); // Receive message from client.

		if(iResult > 0) // If receiving message was successfull.
		{
			// printf("(%i): %s \n", player->ID, recvbuf); Confirmation message moved to function below.
			player->AddMessage(recvbuf); // Message added to queue and deleted.
		}
		else if(iResult == 0)
		{
			std::cout << "Connection closing for ID(" << player->ID << ")." << std::endl;
			player->lost_connection = true;
			delete recvbuf;
			break; // Stop listening for messages.
		}
		else
		{
			std::cout << "RECEIVE failed from ID(" << player->ID << "): " << WSAGetLastError() << std::endl;
			receive_error++;
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}

		if(receive_error > 3) // Problems with connection.
		{
			player->lost_connection = true;
			delete recvbuf;
			break; // Stop listening for messages.
		}
	}
}

/*void Network::SendMessages(Player* player)
{
	//iSendResult = send(*player->GetSocket(), recvbuf, iResult, 0); // Echo the buffer back to the sender.
	//if(iSendResult == SOCKET_ERROR)
	//std::cout << "SEND failed to (" << player->ID << "): " << WSAGetLastError() << std::endl;
	//if(iResult == iSendResult) // See if byte amounts match for received and sent.
	//std::cout << "Connection established with ID(" << player->ID << ")." << std::endl;
	//; // Temporary.
	//else
	//std::cout << "Possible connection problems with ID(" << player->ID << ")." << std::endl;
}*/

void Network::Clean()
{
	WSACleanup(); // Possible functions needed to clean network functionality.
}

int Network::ConnectedPlayers()
{
	std::lock_guard<std::mutex> guard(*lock);
	return players.size();
}

void Network::PushPlayer(Player* player)
{
	std::lock_guard<std::mutex> guard(*lock);
	players.push_back(player);
}

void Network::SendAll(std::string message)
{
	std::lock_guard<std::mutex> guard(*lock);

	for(auto& player : players)
		SendPlayer(player, message);
}

bool Network::SendPlayer(Player* player, std::string message)
{
	if(player->lost_connection)
		return false;

	// std::lock_guard<std::mutex> guard(*lock);

	char* sendbuf = new char[DEFAULT_BUFLEN]();
	strcpy(sendbuf, message.c_str());

	int iResult = send(*player->GetSocket(), sendbuf, DEFAULT_BUFLEN, 0);
	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed to ID(%i) with error: %d \n", player->ID, WSAGetLastError());
		player->lost_connection = true;
		delete sendbuf;
		return false;
	}
	
	delete sendbuf;
	return true;
}

void Network::LobbyCheck()
{
	std::lock_guard<std::mutex> guard(*lock);
	for(auto it = players.begin(); it != players.end();)
	{
		if((*it)->lost_connection)
		{
			std::cout << "Player (" << (*it)->ID << ") has disconnected." << std::endl;
			delete (*it); // Player cleans up after its own socket.
			it = players.erase(it);
		}
		else
			it++;
	}
}

int Network::PlayerAmount()
{
	std::lock_guard<std::mutex> guard(*lock);
	return PLAYERS.size();
}

int Network::UpdatePlayers()
{
	std::lock_guard<std::mutex> guard(*lock);

	int playing = 0;
	for(auto& player : players)
	{
		player->SetPlaying(true);
		playing++;
	}

	return playing;
}

void Network::ClearMessages()
{
	std::lock_guard<std::mutex> guard(*lock);
	for(auto& player : players)
		player->ClearMessages();
}

Network::~Network()
{
}