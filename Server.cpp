#include "Player.h"
#include "Dealer.h"
#include "Network.h"
#include <iostream>
#include <thread>

void AskBet(Player* player); // How much player wants to bet from his available money.
void PlayTurn(Player* player); // Play turn of player until he passes or handvalue exceeds 21.
void UpdatePlayer(Player* player, bool won); // Update money of players and kick poor players from server.

std::vector<Player*> players; // Players who have connected to game.
Dealer dealer; // Handvalues are compared to dealers to determine winners. Also contains the standard 52-card deck.
Network network; // Handles networking and stores players.

int main()
{
	if(!network.CreateHost())
		return 1;

	std::thread t1(network.ListenConnection());

	dealer.MakeDeck();
	players.push_back(new Player);
	players.push_back(new Player);

	while(true)
	{
		if(dealer.deck.size() <= 15) // Remake deck if it's running out of cards.
		{
			std::cout << "Making new deck of cards." << std::endl;
			dealer.MakeDeck();
		}

		std::vector<Player*> round_players = players; // Players who will be playing this round.

		/* Players place their bets. */
		for(auto it = round_players.begin(); it != round_players.end(); it++)
			AskBet((*it));

		std::cout << "Game starts!" << std::endl;

		/* Starting hand of dealer. */
		dealer.AddCard(dealer.GiveCard());
		dealer.AddCard(dealer.GiveCard());
		std::cout << "Dealers starting hand is " << dealer.handvalue << "." << std::endl;
		
		/* Loop through the players. */
		std::cout << "Starting player turns." << std::endl;
		for(auto it = round_players.begin(); it < round_players.end();) // No incremental condition on loop.
		{
			PlayTurn((*it)); // Play turn normally.
			if((*it)->handvalue >= 22) // In case player loses.
			{
				UpdatePlayer((*it), false); // Update players money.
				it = round_players.erase(it); // Remove him from this round and update iterator.
			}
			else
				it++;
		}

		if(round_players.empty()) // Every player bust already.
		{
			std::cout << "All players have bust!" << std::endl;
			goto end;
		}

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

		if(dealer.handvalue >= 22) // If dealers busts everyone still on the round wins.
		{
			std::cout << "Dealer busts!" << std::endl;
			for(auto it = round_players.begin(); it != round_players.end(); it++)
				UpdatePlayer((*it), true);
			goto end; // No need to compare handvalues.
		}

		// Compare dealer hand to player hands to determine winners.
		for(auto it = round_players.begin(); it != round_players.end(); it++)
		{
			if(dealer.handvalue >= (*it)->handvalue) // Dealer wins if his handvalue is bigger or in case of tie.
				UpdatePlayer((*it), false);
			else // Player wins.
				UpdatePlayer((*it), true);
		}

	end: // Remove cards from players and end round.
		dealer.ClearHand();
		for(auto it = players.begin(); it != players.end(); it++)
		{
			(*it)->ClearHand();
			(*it)->Print();
		}
		std::cout << "Round ends!" << std::endl;
		std::cout << "-----------" << std::endl;
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
	player->AddCard(dealer.GiveCard());
	player->AddCard(dealer.GiveCard());
	std::cout << "Players starting hand is " << player->handvalue << "." << std::endl;

	if(player->handvalue == 21) // Natural blackjack ends turn.
	{	
		std::cout << "Blackjack!" << std::endl;
		return;
	}

	while(player->handvalue < 22) // Loop until player passes or handvalue exceeds 21.
	{
		int a = player->AskMove();
		if(a == HIT)
		{
			player->AddCard(dealer.GiveCard());
			std::cout << "Players new hand is " << player->handvalue << "." << std::endl;
		}
		else
			break;
	}

	std::cout << "Ending player turn with " << player->handvalue << "." << std::endl;
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