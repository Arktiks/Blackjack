#include "Player.h"
#include <algorithm>
#include <iostream>
#include <assert.h>

unsigned Player::ID_COUNTER = 0;

Player::Player(SOCKET* socket) : money(10), handvalue(0), betvalue(1), bet_set(false), pass(false), playing(false),
	ID(0), ClientSocket(socket), network_errors(0), lost_connection(false)
{
	ID = ID_COUNTER;
	ID_COUNTER++;
}

Player::~Player()
{
	int iResult = shutdown(*ClientSocket, SD_SEND); // Shutdown the connection socket.
	if(iResult == SOCKET_ERROR)
		std::cout << "CloseConnection on ID(" << ID << ") failed: " << WSAGetLastError();
	closesocket(*ClientSocket);
	delete ClientSocket;
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

void Player::Clear()
{
	cards.clear(); // Remove handcards.
	handvalue = 0; 
	bet_set = false;
	pass = false;
	ClearMessages(); // Clear pending messages.
}

/*int Player::AskMove()
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
}*/

void Player::Print()
{
	std::cout << "Player " << ID << " has " << money << " money." << std::endl;
}

void Player::AddMessage(char* message)
{
	std::lock_guard<std::mutex> lock(message_lock);

	messages.push_back(std::string(message));
	std::cout << "(" << ID << "): " << message << std::endl;
	delete message;
}

int Player::CheckBet()
{
	std::lock_guard<std::mutex> lock(message_lock);

	if(bet_set || messages.size() == 0) // Don't execute if bet was already made or there are no messages.
		return -1;

	char numbers[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'}; // Valid bet amounts.
	bool valid = false; // If valid bet was found.
	int bet_amount = 0; // How much player is going to bet.

	for(auto& msg : messages) // Loop through received messages.
	{
		for(int i = 0; i < 9; i++) // Loop through valid chars.
		{
			if(msg[0] == numbers[i]) // If first letter of message matches with something from numbers.
			{
				bet_amount = msg[0] - '0'; // Convert char into int.

				if(bet_amount <= money) // If player has enough money bet is valid.
				{
					valid = true;
					break;
				}
			}
		}
	}

	assert(bet_amount >= 0 && bet_amount <= 9);

	messages.clear();

	if(valid) // Valid bet was found.
	{
		betvalue = bet_amount;
		bet_set = true;
		std::cout << "(" << ID << "): Bet has been set to " << bet_amount << "." << std::endl;
		return bet_amount;
	}

	/*else
	{
		// SendPlayer(player, "You don't have enough money!");
		//std::cout << "Player (" << player->ID << ") tried to bet " << number << " but has only " << player->money << "." << std::endl;
	}*/

	return -1; // No valid bet was found.
}

int Player::CheckMove()
{
	std::lock_guard<std::mutex> lock(message_lock);

	if(pass || messages.size() == 0) // Player has already finished his turn or there are no messages.
		return -1;

	bool valid = false; // If valid move was found.
	int move = 0; // Players chosen move.

	for(auto& msg : messages) // Loop through received messages.
	{
		if(msg[0] == '0' || msg[0] == '1') // If first letter of message matches with something from possible moves.
		{
			move = msg[0] - '0'; // Convert char into int.
			valid = true;
			break;
		}
	}

	/*if(bet != 0 && bet != 1)
	{
		std::cout << "Player (" << player->ID << ") has illegal move: " << bet << std::endl;
		SendPlayer(player, "Illegal move.");
		return;
	}*/

	assert(move == 0 || move == 1);

	messages.clear();

	/* Return players decision and print some info. */
	if(move == PASS)
	{
		pass = true;
		messages.clear();
		std::cout << "(" << ID << "): PASS" << std::endl;
		return PASS;
	}
	else if(move == HIT)
	{
		std::cout << "(" << ID << "): HIT" << std::endl;
		return HIT;
	}
	else
		return -1;
	
	/*int card = dealer.GiveCard();
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
	}*/
}

void Player::ClearMessages(unsigned int amount)
{
	std::lock_guard<std::mutex> lock(message_lock);

	if(messages.empty())
		return;

	if(amount > messages.size()) // Function possibly called in wrong order.
		return;

	if(amount != 0)
	{
		// assert(amount <= messages.size()); // Can't delete more messages than are in the queue.
		messages.erase(messages.begin(), messages.begin() + amount);
	}
	else
		messages.clear();
}

void Player::SetPlaying(bool playing)
{
	std::lock_guard<std::mutex> lock(message_lock);
	this->playing = playing;
}

void Player::SkipRound()
{
	std::lock_guard<std::mutex> lock(message_lock);
	playing = false;
	betvalue = 0;
}

void Player::Error()
{
	std::lock_guard<std::mutex> lock(message_lock);
	network_errors++;
}

SOCKET* Player::GetSocket()
{
	return ClientSocket;
}