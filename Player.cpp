#include "Player.h"
#include <algorithm>
#include <iostream>

unsigned Player::ID_COUNTER = 0;

Player::Player(SOCKET* socket) : money(10), handvalue(0), betvalue(1), ID(0), ClientSocket(socket)
{
	ID = ID_COUNTER;
	ID_COUNTER++;
}

int Player::AddCard(int card)
{
	cards.push_back(card);
	return CalculateHand();
}

int Player::CalculateHand()
{
	int sum = 0;
	for(auto& card : cards) // Sum all cards together.
		sum += card;

	if(std::find(cards.begin(), cards.end(), 1) != cards.end() && // Player has an ace.
	   sum < 12) // Ace can be count as 11 instead of 1.
	   sum += 10;

	handvalue = sum;
	return sum;
}

void Player::ClearHand()
{
	cards.clear();
	handvalue = 0;
}

int Player::AskMove()
{
	int ask = 0;
	std::cout << "Pass (0) or Hit (1)?" << std::endl;
	while(true)
	{
		std::cin >> ask;
		if(ask == 0 || ask == 1)
		{
			fflush(stdin);
			return ask;
		}
	}
}

void Player::Print()
{
	std::cout << "Player " << ID << " has " << money << " money." << std::endl;
}

void Player::CloseConnection()
{
	int iResult = shutdown(*ClientSocket, SD_SEND); // Shutdown the connection since we're done.
	if(iResult == SOCKET_ERROR)
		std::cout << "CloseConnection on ID(" << ID << ") failed: " << WSAGetLastError();

	closesocket(*ClientSocket);
}