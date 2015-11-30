#pragma once
#include "Player.h"
#include <vector>
#include <mutex>

class Dealer : public Player
{
public:
	Dealer();
	~Dealer() {};

	/* Remove card from deck and return its value.
	* Return: Value of removed card. */
	int GiveCard();

	/* Create deck of 52 cards without specific suits. */
	void MakeDeck();

	/* Add another deck of cards into the deck. */
	void AddDeck();

	/* Ask if dealer would like to hit or pass. */
	int AskMove();

	std::vector<int> deck; // Cards left on the deck.

	std::mutex deck_lock;
};