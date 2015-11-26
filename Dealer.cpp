#include "Dealer.h"
#include <algorithm>
#include <iostream>

Dealer::Dealer() : Player(nullptr)
{
	MakeDeck();
}

int Dealer::GiveCard()
{
	int card = deck.back();
	std::cout << card << std::endl;
	deck.pop_back();
	return card;
}

void Dealer::MakeDeck()
{
	deck.clear();
	for(int i = 1; i < 9; i++) // Cards ranging from 1 to 9.
	{
		for(int j = 0; j < 4; j++)
			deck.push_back(i);
	}

	for(int i = 0; i < 16; i++) // Tens and picture cards.
		deck.push_back(10);

	std::random_shuffle(deck.begin(), deck.end()); // Shuffle deck.
}

int Dealer::AskMove()
{
	/* Keeping it simple until networking feature is ready. */
	if(handvalue < 17)
		return HIT;
	return PASS;
}