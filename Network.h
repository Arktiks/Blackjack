#pragma once
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 80
#define DEFAULT_PORT "7500"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <winsock2.h>
#include <vector>
#include <mutex>
#include "Player.h"

class Network
{
public:
	Network();

	Network(Network& network);

	static std::vector<Player*> players; // Connected players.

	static SOCKET ListenSocket; // Socket initialised by CreateHost.

	static std::mutex* lock;

	bool CreateHost(); // Initialise the listening socket.

	void ListenConnection(); // Listen for new clients attempting to connect to server.

	void ListenMessage(Player* player); // Listen for possible messages from established clients.

	void SendMessages(Player* player);

	void Clean(); // Clean up networking functionality.

	int ConnectedPlayers(); // How many players have connected.

	void PushPlayer(Player* player); // Threadsafe push_back.

	

	// void ClearBuffer(char (&buffer)[10]); // Clear message buffer.

	//void GetSocket(Player* player);
};