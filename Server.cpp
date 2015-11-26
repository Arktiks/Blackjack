#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 80
#define DEFAULT_PORT "7500"

#include "Player.h"
#include "Dealer.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>

#include <windows.h> // Includes for TCP networking.
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

void AskBet(Player* player); // How much player wants to bet from his available money.
void PlayTurn(Player* player); // Play turn of player until he passes or handvalue exceeds 21.
void UpdatePlayer(Player* player, bool won); // Update money of players and kick poor players from server.

std::vector<Player*> players; // Players who have connected to game.
std::vector<Player*> round_players; // Players who are still participating on the current round.
Dealer dealer; // Handvalues are compared to dealers to determine winners. Also contains the standard 52-card deck.

std::mutex lock; // Safethread vector manipulating.
SOCKET ListenSocket; // Socket used to initialise new connections to server.

bool CreateHost(); // Initialise the listening socket.
void ListenConnection(); // Listen for new clients attempting to connect to server.
void ListenMessage(Player* player); // Listen for possible messages from established clients.
void SendAll(std::string message); // Send all players a message.
bool SendPlayer(Player* player, std::string message); // Send single player a message.
void LobbyCheck(); // See if there are disconnected players that should be deleted.
int PlayerAmount(); // Return how many players have connected to server.

void CheckPlayerBet(Player* player, char bet); // Take player input and make sure it's valid.
void CheckBets(); // See if all players have set their bets so game can continue.

void CheckPlayerMove(Player* player, char move);
void CheckMoves();

bool betting = false; // Is it time to set bets.
bool make_hand = false; // Is it time to make decisions.

int main()
{
	if(!CreateHost()) // Abort program if listening socket can't be established.
	{
		std::cin.get();
		return 1;
	}

	std::thread listen_thread(ListenConnection); // Start listening for new players.
	listen_thread.detach();

	dealer.MakeDeck(); // Create deck of 52 cards.

	while(true) // Main gameloop.
	{
		if(PlayerAmount() >= 1) // If there are enough players start the round.
		{
			if(dealer.deck.size() <= 15) // Remake deck if it's running out of cards.
			{
				std::cout << "Making new deck of cards." << std::endl;
				dealer.MakeDeck();
			}


			/* Players place their bets. */
			std::cout << "Game starts!" << std::endl;
			SendAll("New round is starting! Set your bets (1-9).");
			betting = true; // Start betting phase.
			
			CheckBets(); // Blocking call giving time for players to make their bets.
			betting = false; // End betting phase.

			SendAll("Bets are in! Starting dealers turn.");


			/* Starting hand of dealer. */
			int card = dealer.GiveCard();
			std::string cn = std::to_string(card); // Card-number.
			dealer.AddCard(card);
			SendAll(cn);

			card = dealer.GiveCard();
			cn = std::to_string(card);
			dealer.AddCard(card);
			SendAll(cn);

			std::cout << "Dealers starting hand is " << dealer.handvalue << "." << std::endl;
			cn = std::to_string(dealer.handvalue);
			SendAll("Dealers starting hand is " + cn);


			/* Loop through the player turns. */
			std::cout << "Starting player turns." << std::endl;
			make_hand = true;

			for(auto it = round_players.begin(); it < round_players.end(); it++)
				PlayTurn((*it)); // Play turn normally.

			CheckMoves(); // Blocking call giving time for players to take their moves.
			make_hand = false;

			/*if(round_players.empty()) // Every player bust already.
			{
				std::cout << "All players have bust!" << std::endl;
				goto end;
			}*/


			/* Go through dealers turn. */
			std::cout << "Starting dealers turns." << std::endl;
			while(dealer.handvalue < 17)
			{
				int b = dealer.AskMove();
				if(b == HIT)
				{
					std::cout << "Dealer hits." << std::endl;
					dealer.AddCard(dealer.GiveCard());
				}
				else
					break;
			}
			std::cout << "Ending dealers turn with " << dealer.handvalue << "." << std::endl;
			cn = std::to_string(dealer.handvalue);
			SendAll("Ending dealers turn with: " + cn);


			/* Deal with winnings if dealer busts. */
			if(dealer.handvalue >= 22) // If dealers busts everyone still on the round wins.
			{
				std::cout << "Dealer busts!" << std::endl;
				for(auto it = round_players.begin(); it != round_players.end(); it++)
				{
					if((*it)->handvalue >= 22) // Player bust.
						UpdatePlayer((*it), false);
					else
						UpdatePlayer((*it), true);
				}
				goto end; // No need to compare handvalues.
			}


			/* Compare dealer hand to player hands to determine winners. */
			for(auto it = round_players.begin(); it != round_players.end(); it++)
			{
				if((*it)->handvalue >= 22) // Player bust.
					UpdatePlayer((*it), false);
				else if(dealer.handvalue >= (*it)->handvalue) // Dealer wins if his handvalue is bigger or in case of tie.
					UpdatePlayer((*it), false);
				else // Player wins.
					UpdatePlayer((*it), true);
			}


		end: /* Remove cards from players and end round. */
			dealer.ClearHand();
			for(auto it = players.begin(); it != players.end(); it++)
			{
				(*it)->ClearHand();
				(*it)->Print();
			}
			std::cout << "Round ends!" << std::endl;
			std::cout << "-----------" << std::endl;
		}
		else // There are not enough players.
		{
			std::cout << "There are " << PlayerAmount() << " players connected." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
}

void AskBet(Player* player)
{
	std::cout << "You have " << player->money << " money left. Set your bet:" << std::endl;
	int bet = 0;
	while(bet <= 0 || bet > player->money) // Make sure bet value is legit.
		std::cin >> bet;
	fflush(stdin);
	player->betvalue = bet; // Set players bet.
}

void PlayTurn(Player* player)
{
	std::cout << "Handing player starting cards." << std::endl; // Give player two cards.
	int card = dealer.GiveCard();
	std::string cn = std::to_string(card); // Card-number.
	player->AddCard(card);
	SendPlayer(player, cn);

	card = dealer.GiveCard();
	cn = std::to_string(card);
	player->AddCard(card);
	SendPlayer(player, cn);

	std::cout << "Player ID(" << player->ID << ") starting hand is " << player->handvalue << "." << std::endl;
	cn = std::to_string(player->handvalue);
	SendPlayer(player, "Your starting hand is: " + cn);

	if(player->handvalue == 21) // Natural blackjack ends turn.
	{	
		std::cout << "Player ID(" << player->ID << ")" << "Blackjack!" << std::endl;
		SendPlayer(player, "Blackjack!");
		player->pass = true;
		return;
	}

	/*while(player->handvalue < 22) // Loop until player passes or handvalue exceeds 21.
	{
		int a = player->AskMove();
		if(a == HIT)
		{
			player->AddCard(dealer.GiveCard());
			std::cout << "Player ID(" << player->ID << ") new hand is " << player->handvalue << "." << std::endl;
		}
		else
			break;
	}

	std::cout << "Ending player ID(" << player->ID << ") turn with " << player->handvalue << "." << std::endl;*/

	SendPlayer(player, "Pass turn (0) or Hit another card (1).");
}

void UpdatePlayer(Player* player, bool won)
{
	if(won)
	{
		player->money += player->betvalue; // Add bet to players money.
		std::cout << "Player " << player->ID << " wins!" << std::endl;
	}
	else
	{
		player->money -= player->betvalue; // Remove bet from players money.
		std::cout << "Player " << player->ID << " loses." << std::endl;

		if(player->money <= 0) // If player runs out of money kick him from server.
		{
			for(auto it = players.begin(); it != players.end(); it++)
			{
				if((*it) == player)
				{
					std::cout << "Player " << player->ID << " has run out of money." << std::endl;
					//delete (*it);
					// Function to kick from server.
					return;
				}
			}
		}
	}
}

bool CreateHost()
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
}

void ListenConnection()
{
	while(true)
	{
		SOCKET* ClientSocket = new SOCKET(INVALID_SOCKET);

		*ClientSocket = accept(ListenSocket, NULL, NULL); // Blocking call waiting for client socket.

		if(*ClientSocket == INVALID_SOCKET) // Error initialising the socket.
		{
			printf("ListenConnection failed with error: %d\n", WSAGetLastError());
		}
		else // Initialise new player with the created socket.
		{
			Player* player = new Player(ClientSocket);
			lock.lock();
			players.push_back(player); // Create new player upon successful connection.
			lock.unlock();
			std::cout << "New player connected ID(" << player->ID << ")." << std::endl;

			std::thread message_thread(ListenMessage, player); // Start new thread for communication.
			message_thread.detach();
		}
	}
}

void ListenMessage(Player* player)
{
	std::cout << "Starting ListenMessage for ID(" << player->ID << ")." << std::endl;
	
	SOCKET* socket = player->GetSocket(); // Handle to players socket.

	int receive_error = 0;

	while(true) // Receive messages until the peer shuts down the connection.
	{
		char recvbuf[DEFAULT_BUFLEN] = {0}; // Buffer for messages.

		int iResult = recv(*socket, recvbuf, DEFAULT_BUFLEN, 0); // Receive message from client.

		if(iResult > 0) // If receiving message was successfull.
		{
			printf("(%i): %s \n", player->ID, recvbuf);

			if(betting) // If game is on its betting phase.
				CheckPlayerBet(player, recvbuf[0]);

			if(make_hand) // If game is on deciding phase.
				CheckPlayerMove(player, recvbuf[0]);
		}
		else if(iResult == 0)
		{
			std::cout << "Connection closing for ID(" << player->ID << ")." << std::endl;
			player->lost_connection = true;
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
			break; // Stop listening for messages.
		}
	}
}

void SendAll(std::string message)
{
	lock.lock();
	for(auto& player : round_players)
		SendPlayer(player, message);
	lock.unlock();
}

bool SendPlayer(Player* player, std::string message)
{
	char sendbuf[DEFAULT_BUFLEN];
	strcpy(sendbuf, message.c_str());
	int iResult = send(*player->GetSocket(), sendbuf, DEFAULT_BUFLEN, 0);
	if(iResult == SOCKET_ERROR)
	{
		printf("SEND failed to ID(%i) with error: %d \n", player->ID, WSAGetLastError());
		return false;
	}
	return true;
}

void LobbyCheck()
{
	lock.lock();
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
	lock.unlock();
}

int PlayerAmount()
{
	return players.size();
}

void CheckPlayerBet(Player* player, char bet)
{
	if(player->bet_set) // Don't execute if bet was already made.
		return;

	char numbers[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
	bool valid = false;

	for(int i = 0; i < 9; i++) // See if received input is valid.
	{
		if(bet == numbers[i])
		{
			valid = true;
			break;
		}
	}

	if(!valid)
	{
		std::cout << "Player (" << player->ID << ") has illegal bet: " << bet << std::endl;
		return;
	}

	int number = bet - '0'; // Conver char into int.

	if(number <= player->money) // Player has enough money.
	{
		lock.lock();
		player->betvalue = number;
		player->bet_set = true;
		lock.unlock();
	}
	else
	{
		SendPlayer(player, "You don't have enough money!");
		std::cout << "Player (" << player->ID << ") tried to bet " << number << " but has only " << player->money << "." << std::endl;
	}
}

void CheckBets()
{
	int loops = 0; // How many times function has been looped.

	while(true)
	{
		std::cout << "Waiting for bets." << std::endl;
		int ready = 0;
		
		lock.lock();
		for(auto& player : round_players)
		{
			if(player->bet_set)
				ready++;
		}

		if(PlayerAmount() == ready) // All players are ready.
		{
			lock.unlock();
			break;
		}
		else if(loops >= 3) // Players are taking too much time.
		{
			for(auto& player : round_players)
			{
				if(player->bet_set == false) // Unset bets are 1.
				{
					player->bet_set = true;
					player->betvalue = 1;
				}
			}

			lock.unlock();
			break;
		}
		lock.unlock();

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void CheckPlayerMove(Player* player, char bet)
{
	if(player->pass) // Player has already finished.
		return;

	if(bet != 0 && bet != 1)
	{
		std::cout << "Player (" << player->ID << ") has illegal move: " << bet << std::endl;
		SendPlayer(player, "Illegal move.");
		return;
	}

	int number = bet - '0';

	if(bet == PASS)
	{
		player->pass;
		return;
	}

	int card = dealer.GiveCard();
	std::string cn = std::to_string(card); // Card-number.
	player->AddCard(card);
	SendPlayer(player, cn);

	std::cout << "Player ID(" << player->ID << ") new hand is " << player->handvalue << "." << std::endl;
	cn = std::to_string(player->handvalue);
	SendPlayer(player, "Your new hand is: " + cn);

	if(player->handvalue == 21) // Blackjack ends turn.
	{
		std::cout << "Player ID(" << player->ID << ")" << "Blackjack!" << std::endl;
		SendPlayer(player, "Blackjack!");
		player->pass = true;
		return;
	}
	else if(player->handvalue > 22)
	{
		std::cout << "Player ID(" << player->ID << ")" << "Bust!" << std::endl;
		SendPlayer(player, "You have bust.");
		player->pass = true;
		return;
	}
}

void CheckMoves()
{
	int loops = 0; // How many times function has been looped.

	while(true)
	{
		std::cout << "Waiting for moves." << std::endl;
		int ready = 0;

		lock.lock();
		for(auto& player : round_players)
		{
			if(player->pass)
				ready++;
		}

		if(PlayerAmount() == ready) // All players are ready.
		{
			lock.unlock();
			break;
		}
		else if(loops >= 3) // Players are taking too much time.
		{
			for(auto& player : round_players)
				player->pass = true;

			lock.unlock();
			break;
		}
		lock.unlock();

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}