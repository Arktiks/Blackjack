#pragma once
#include <vector>

enum MOVES { // Possible moves player can take can be extended later.
	PASS,
	HIT
};

class Player
{
public:
	Player();
	// Player(int money) : money(money), handvalue(0), betvalue(1) {};
	virtual ~Player() {};

	/* Add card to players handcards.
	* Return: New total value of hand. */
	int AddCard(int card);

	/* Calculate total value of hand taking aces into account.
	* Updates handvalue variable.
	* Return: Total value of hand. */
	int CalculateHand();

	/* Clear cards and reset handvalue.	*/
	void ClearHand();

	/* Ask if player would like to HIT or PASS. */
	virtual int AskMove();

	/* Print player statistics. */
	virtual void Print();

	std::vector<int> cards; // Cards player has on his hand.
	int money; // How much money player has left.
	int handvalue; // Sum of current hand cards.
	int betvalue; // How much player is betting on current round.

	unsigned ID; // Unique ID used to differentiate players.
	static unsigned ID_COUNTER; // Increment ID during constructor.
};