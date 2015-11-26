#pragma once
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 80
#define DEFAULT_PORT "7500"

#pragma comment (lib, "Ws2_32.lib")

#include <winsock2.h>
#include <vector>
#include "Player.h"

class Network
{
public:
	Network();

	bool CreateHost(); // Initialise the listening socket.

	void ListenConnection(); // Listen for new clients attempting to connect to server.

	void ListenMessage(Player* player); // Listen for possible messages from established clients.

	void Clean(); // Clean up networking functionality.

	void ClearBuffer(char (&buffer)[10]); // Clear message buffer.

	SOCKET ListenSocket;

	std::vector<Player*> players;

	int ConnectedPlayers(); // How many players have connected.
};