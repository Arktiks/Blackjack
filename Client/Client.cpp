#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <mutex>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 60
#define DEFAULT_PORT "7500"

void ReceiveMessages(SOCKET socket); // Receive messages from server.
bool WriteMessage(); // Takes user input and sends it to server.
void Error(); // Increase error count.

SOCKET ConnectSocket = INVALID_SOCKET; // Socket used to connect to server.
int errors = 0; // Keep threshold for amount of errors there can be sending messages.
bool receiving = true; // Is client still receiving messages.
std::mutex lock; // Safethread error handling.

int main()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock.
	if(iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//hints.ai_addr = DEFAULT_SERVER;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); // Resolve the server address and port
	if(iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	for(ptr = result; ptr != NULL; ptr = ptr->ai_next) // Attempt to connect to an address until one succeeds.
	{
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // Create a SOCKET for connecting to server.
		if(ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen); // Connect to server.
		if(iResult == SOCKET_ERROR)
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if(ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	else
		std::cout << "Server connection established." << std::endl;

	std::cout << "Sending confirmation message to server." << std::endl;

	char sendbuf[] = "CONFIRM CONNECTION!";
	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf), 0); // Send an initial buffer.

	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else
		std::cout << "Confirmation message was successfull." << std::endl;

	std::thread receive(ReceiveMessages, ConnectSocket); // Start thread for receiving messages.
	receive.detach();

	while(WriteMessage() == true && receiving); // Send messages to server.

	shutdown(ConnectSocket, SD_SEND); 
	closesocket(ConnectSocket);
	WSACleanup();

	fflush(stdin);
	std::cin.get();

	return 0;
}

void ReceiveMessages(SOCKET socket)
{
	while(true)
	{
		char recvbuf[DEFAULT_BUFLEN] = {0}; // Buffer for messages.

		int iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0); // Blocking call for receiving messages.

		if(iResult > 0) // Print the received message.
		{
			printf("%s \n", recvbuf);
		}
		else if(iResult == 0)
		{
			printf("Connection closed! \n");
			receiving = false;
			return;
		}
		else // Error receiving message from server.
		{
			Error();
			printf("RECEIVE failed with error: %d \n", WSAGetLastError());
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		if(errors >= 3) // Too many connection errors, discontinue receiving messages.
		{
			receiving = false;
			std::cout << "Stopping ReceiveMessages thread." << std::endl;
			return;
		}
	}
}

bool WriteMessage()
{
	char sendbuf[DEFAULT_BUFLEN];
	gets(sendbuf);
	int iResult = send(ConnectSocket, sendbuf, DEFAULT_BUFLEN, 0);

	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed with error: %d \n", WSAGetLastError());
		return false;
	}

	return true;
}

void Error()
{
	lock.lock();
	errors++;
	std::cout << "Problems with connection, increasing error count (" << errors << ")." << std::endl;
	lock.unlock();
}

/*void SendMessages(std::string message)
{
	unsigned size = message.size(); // How long message is.
	char* send_buffer = new char[size]; // Make message based on length.
	strcpy(send_buffer, message.c_str());

	int iResult = send(ConnectSocket, send_buffer, size, 0); // Send the message.
	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed with error: %d\n", WSAGetLastError());
		errors++;
		// Possibly break the loop and close client.
	}

	delete send_buffer;
}*/