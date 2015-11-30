#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 80
#define DEFAULT_PORT "7500"

#include "Player.h"
#include "Dealer.h"
#include "Network.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <assert.h>

#include <windows.h> // Includes for TCP networking.
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

const int LOOPS = 5; // How much time players have to set bets or make moves.
const int LOOP_TIME = 2; // How many seconds each loop should sleep.
int round_players = 0; // How many players are participating on the current round.

void AskBet(Player* player); // How much player wants to bet from his available money.
void PlayTurn(Player* player); // Play turn of player until he passes or handvalue exceeds 21.
void UpdatePlayer(Player* player, bool won); // Update money of players and kick poor players from server.
void KickPlayers(); // Remove players who don't have money or have lost connection.

Dealer dealer; // Handvalues are compared to dealers to determine winners. Also contains the standard 52-card deck.
Network network;

void CheckBets(); // See if all players have set their bets so game can continue.
void CheckMoves(); // See if all players have passed so game can continue.

int main()
{
	if(!network.CreateHost()) // Abort program if listening socket can't be established.
	{
		std::cin.get();
		return 1;
	}

	std::thread listen_thread(&Network::ListenConnection, &network); // Start listening for new players.
	listen_thread.detach(); // Program might terminate before thread finishes.

	dealer.MakeDeck(); // Create deck of 52 cards.

	while(true) // Main gameloop.
	{
		if(network.PlayerAmount() >= 1) // If there are enough players start the round.
		{
			round_players = network.UpdatePlayers(); // Update which players are participating this round.
			
			if(dealer.deck.size() <= 15) // Remake deck if it's running out of cards.
			{
				std::cout << "Making new deck of cards." << std::endl;
				dealer.MakeDeck();
			}


			/*** Players place their bets. ***/
			std::cout << "New round starting with " << round_players << " players!" << std::endl;
			network.SendAll("New round is starting! Set your bets (1-9).");
			network.ClearMessages();

			std::vector<std::thread> bet_threads; 
			for(auto it = network.players.begin(); it < network.players.end(); it++) // Start betting thread for each player.
			{
				if((*it)->playing) // If player is participating this round.
					bet_threads.push_back(std::thread(AskBet, (*it))); // Start his betting.
			}

			for(auto& thread : bet_threads) // Join the betting threads.
				thread.join();

			network.SendAll("Bets are in! Starting dealers turn.");
			std::this_thread::sleep_for(std::chrono::seconds(2));


			/*** Starting hand of dealer. ***/
			int card = dealer.GiveCard(); // Send information of drawn cards to players as well.
			dealer.AddCard(card);
			network.SendAll(std::to_string(card));

			card = dealer.GiveCard();
			dealer.AddCard(card);
			network.SendAll(std::to_string(card));

			std::cout << "Dealers starting hand is " << dealer.handvalue << "." << std::endl;
			network.SendAll("Dealers starting hand is " + std::to_string(dealer.handvalue) + ".");
			std::this_thread::sleep_for(std::chrono::seconds(2));


			/*** Loop through the player turns. ***/
			std::cout << "Starting player turns." << std::endl;
			network.SendAll("Dealers turn is over. Starting player turns.");
			network.ClearMessages();

			std::vector<std::thread> turn_threads; // Each players turn is processed in different thread.
			for(auto it = network.players.begin(); it < network.players.end(); it++) // Start turn thread for each player.
			{
				if((*it)->playing) // If player is participating this round.
					turn_threads.push_back(std::thread(PlayTurn, (*it))); // Start his turn.
			}

			for(auto& thread : turn_threads) // Join the game threads.
				thread.join();

			
			/*** Go through dealers turn. ***/
			std::cout << "Starting dealers turns." << std::endl;
			network.SendAll("Player turns are over! Starting dealers turn.");

			while(dealer.handvalue < 17) // Dealer AI.
			{
				int b = dealer.AskMove();
				if(b == HIT)
				{
					std::cout << "Dealer hits." << std::endl;
					int d = dealer.GiveCard(); // Dealers card.
					dealer.AddCard(d);
					network.SendAll("Dealer: " + std::to_string(d));
					std::this_thread::sleep_for(std::chrono::seconds(2));
				}
				else
					break;
			}

			std::cout << "Ending dealers turn with " << dealer.handvalue << "." << std::endl;
			network.SendAll("Ending dealers turn with: " + std::to_string(dealer.handvalue));
			std::this_thread::sleep_for(std::chrono::seconds(2));


			/*** Deal with winnings if dealer busts. ***/
			if(dealer.handvalue >= 22) // If dealers busts everyone still on the round wins.
			{
				std::cout << "Dealer busts!" << std::endl;
				network.SendAll("Dealer busts!");

				for(auto it = network.players.begin(); it != network.players.end(); it++)
				{
					if(!(*it)->playing) // If player is playing.
						continue;

					if((*it)->handvalue >= 22) // Player bust.
						UpdatePlayer((*it), false);
					else
						UpdatePlayer((*it), true);
				}
				goto end; // No need to compare handvalues.
			}


			/*** Compare dealer hand to player hands to determine winners. ***/
			for(auto it = network.players.begin(); it != network.players.end(); it++)
			{
				if(!(*it)->playing) // If player is playing.
					continue;

				if((*it)->handvalue >= 22) // Player bust.
					UpdatePlayer((*it), false);
				else if(dealer.handvalue >= (*it)->handvalue) // Dealer wins if his handvalue is bigger or in case of tie.
					UpdatePlayer((*it), false);
				else // Player wins.
					UpdatePlayer((*it), true);
			}


		end: /*** Remove cards from players and end round. ***/
			dealer.Clear(); // Clear dealers hand.
			KickPlayers(); // Kick unwanted players from table.

			for(auto it = network.players.begin(); it != network.players.end(); it++) // Prepare players for next round.
			{
				(*it)->Clear();
				(*it)->Print();
			}

			std::cout << "Round ends!" << std::endl;
			std::cout << "-----------" << std::endl;
			network.SendAll("-----------");

			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		else // There are not enough players.
		{
			std::cout << "There are " << network.PlayerAmount() << " players connected." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
}

void AskBet(Player* player)
{
	int loops_done = 0; // Keep track of players betting time.

	network.SendPlayer(player, "You have " + std::to_string(player->money) + " money.");

	std::this_thread::sleep_for(std::chrono::seconds(2));

	while(true)
	{
		// Loop through player messages and see if there was a valid bet. Otherwise clear players message queue.
		int bet = player->CheckBet();

		if(bet != -1) // Player has succesfully set his bet.
		{
			network.SendPlayer(player, "Your bet has been set to " + std::to_string(bet) + ".");
			return;
		}

		std::this_thread::sleep_for(std::chrono::seconds(LOOP_TIME));
		loops_done++;

		if(loops_done >= LOOPS) // Player has taken too much time.
		{
			network.SendPlayer(player, "Player took too long, skipping this round.");
			player->SkipRound();
			return;
		}
	}
}

void PlayTurn(Player* player)
{
	// Give player his two starting cards.
	int card = dealer.GiveCard();
	player->AddCard(card);
	network.SendPlayer(player, std::to_string(card));

	card = dealer.GiveCard();
	player->AddCard(card);
	network.SendPlayer(player, std::to_string(card));

	// Send information to server and player.
	std::cout << "Player ID(" << player->ID << ") starting hand is " << player->handvalue << "." << std::endl;
	network.SendPlayer(player, "Your starting hand is: " + std::to_string(player->handvalue));

	if(player->handvalue == 21) // Natural blackjack ends turn.
	{
		std::cout << "Player ID(" << player->ID << ")" << "Blackjack!" << std::endl;
		network.SendPlayer(player, "Blackjack!");
		player->pass = true;
		return;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	network.SendPlayer(player, "Pass turn (0) or Hit another card (1).");
	int loops_done = 0; // Keep track of players turn time.

	while(player->handvalue < 22) // Loop until player passes or handvalue exceeds 21.
	{
		int move = player->CheckMove();

		if(move == HIT) // Give player new card.
		{
			int card = dealer.GiveCard();
			player->AddCard(card);
			network.SendPlayer(player, std::to_string(card));

			std::cout << "Player ID(" << player->ID << ") new hand is " << player->handvalue << "." << std::endl;
			network.SendPlayer(player, "Your new hand is: " + std::to_string(player->handvalue));
		}
		else if(move == PASS) // End players turn.
		{
			player->pass = true;
			network.SendPlayer(player, "Ending turn with: " + std::to_string(player->handvalue));
			return;
		}

		std::this_thread::sleep_for(std::chrono::seconds(LOOP_TIME));
		loops_done++;

		if(loops_done >= (LOOPS * 2)) // Player has taken too much time.
		{
			network.SendPlayer(player, "Ending turn with current hand.");
			return;
		}
	}

	player->pass = true;
	std::cout << "Ending player ID(" << player->ID << ") turn with " << player->handvalue << "." << std::endl;
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

		/*if(player->money <= 0) // If player runs out of money kick him from server.
		{
			for(auto it = network.players.begin(); it != network.players.end(); it++)
			{
				if((*it) == player)
				{
					std::cout << "Player " << player->ID << " has run out of money." << std::endl;
					//delete (*it);
					// Function to kick from server.
					//return;
				}
			}
		}*/
	}
}

void KickPlayers()
{
	for(auto it = network.players.begin(); it != network.players.begin();)
	{
		if((*it)->money <= 0 || (*it)->lost_connection)
		{
			std::cout << "Kicking player ID(" << (*it)->ID << ")!" << std::endl;
			delete (*it);
			it = network.players.erase(it);
		}
		else
			it++;
	}
}