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

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 60
#define DEFAULT_PORT "7500"
#define DEFAULT_SERVER "127.0.0.1"

void ReceiveMessages(SOCKET socket); // Receive messages from server.
void SendMessages(std::string message); // Send messages to server with proper formatting.
SOCKET ConnectSocket = INVALID_SOCKET; // Socket used to connect to server.
int errors = 0; // Keep threshold for amount of errors there can be sending messages.

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

	std::this_thread::sleep_for(std::chrono::seconds(2));

	std::cout << "Sending confirmation message to server." << std::endl;

	char sendbuf[] = "TEST";
	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf), 0); // Send an initial buffer.

	if(iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else
		std::cout << "Confirmation message was successfull." << std::endl;

	//printf("Bytes Sent: %ld\n", iResult);

	std::thread receive(ReceiveMessages, ConnectSocket); // Start thread for receiving messages.

	std::this_thread::sleep_for(std::chrono::seconds(2));

	

	while(true) // Send messages.
	{
		std::string message;
		std::cout << "Send message to server (max 10): " << std::endl;
		fflush(stdin); // Flush std::cin buffer.

		while(message.size() > 10 || message.size() == 0) // Confirm that message is valid.
			std::getline(std::cin, message);

		SendMessages(message);

		if(errors >= 3) // There have been too many errors sending messages.
		{
			std::cout << "Lost connection to server." << std::endl;
			fflush(stdin);
			std::cin.get();
			break;
		}
	}

	shutdown(ConnectSocket, SD_SEND);
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

void ReceiveMessages(SOCKET socket)
{
	int iResult = 0; // Test if receive was successfull.
	char recvbuf[DEFAULT_BUFLEN]; // Buffer for messages.
	int recvbuflen = DEFAULT_BUFLEN; // Buffer length for messages.
	int errors = 0;

	while(true)
	{
		//iResult = recv(socket, recvbuf, recvbuflen, 0);
		iResult = recv(socket, recvbuf, recvbuflen, 0);

		std::string received_message(recvbuf);
		int size = received_message.find('\0'); // Find newline.
		received_message.resize(size); // Resize message to actual size.

		if(iResult > 0) // Print the received message.
		{
			std::string message(recvbuf);
			std::cout << message << std::endl;
			//printf("Bytes received: %d\n", iResult);
		}
		else if(iResult == 0)
		{
			printf("Connection closed!\n");
			// Cleaning functions placeholder.
			return;
		}
		else // Error receiving message from server.
		{
			errors++;
			printf("RECEIVE failed with error: %d\n", WSAGetLastError());
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}

		if(errors >= 3)
		{
			std::cout << "Stopping ReceiveMessages thread." << std::endl;
			return;
		}
	}
}

void SendMessages(std::string message)
{
	int size = message.size() + 1;

	char* send_buffer = new char[size];
	//char send_buffer[size]; // Transform std::string back to char-array.

	strcpy(send_buffer, message.c_str());
	send_buffer[size] = '\0'; // Make sure message has endline.

	int iResult = send(ConnectSocket, send_buffer, size, 0); // Send the message.
	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed with error: %d\n", WSAGetLastError());
		errors++;
		// Possibly break the loop and close client.
	}

	delete send_buffer;
}