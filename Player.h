#pragma once
#include <vector>
#include <winsock2.h>

enum MOVES { // Possible moves player can take - extendable for later.
	PASS,
	HIT
};

class Player
{
public:

	Player(SOCKET* socket); // Players needs to be initialised with connection socket.

	virtual ~Player();

	/* Add card to players handcards.
	* Return: New total value of hand. */
	int AddCard(int card);

	/* Calculate total value of hand taking aces into account.
	* Updates handvalue variable.
	* Return: Total value of hand. */
	int CalculateHand();

	void ClearHand(); // Clear cards and reset handvalue.

	virtual int AskMove(); // Ask if player would like to HIT or PASS.

	virtual void Print(); // Print player statistics.

	SOCKET* GetSocket(); // Return handle to socket.

	std::vector<int> cards; // Cards player has on his hand.
	int money; // How much money player has left.
	int handvalue; // Sum of current hand cards.
	int betvalue; // How much player is betting on current round.
	bool bet_set; // Has player succesfully set his bet.
	bool pass; // Can player make any further moves.

	unsigned ID; // Unique ID used to differentiate players.
	static unsigned ID_COUNTER; // Increment ID during constructor.

	bool lost_connection; // Has player lost connection to server.

	SOCKET* ClientSocket;
};