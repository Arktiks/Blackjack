#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h> // Includes for TCP networking.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 10
#define DEFAULT_PORT "7500"

class Network
{
public:
	Network();

	bool CreateHost();



	SOCKET ListenSocket;

	SOCKET ClientSocket;
};