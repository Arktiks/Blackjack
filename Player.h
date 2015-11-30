#pragma once
#include <vector>
#include <winsock2.h>
#include <string>
#include <mutex>

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

	void Clear(); // Clear variables for new round.

	// virtual int AskMove() = 0; // Ask if player would like to HIT or PASS.

	void Print(); // Print player statistics.

	void AddMessage(char* message); // Add message to received messages queue and delete message.

	/* Check message queue for possible bets.
	* Return: -1 if there was no valid bet.
	* Otherwise return bet amount. */
	int CheckBet();

	/* Check message queue for possible moves.
	* Return: -1 if there was no valid move.
	* Otherwise return players move. */
	int CheckMove();

	/* Clear messages from the queue.
	* amount: How many messages should be deleted starting from messages.begin().
	* amount: 0 all messages will be cleared. */
	void ClearMessages(unsigned int amount = 0); 

	void SetPlaying(bool playing); // Set if player is participating on current round.

	void SkipRound(); // Skip round if no bet was set during betting time.

	void Error(); // Should be called when there is problems within player connection.

	SOCKET* GetSocket(); // Return handle to socket.

	std::vector<int> cards; // Cards player has on his hand.
	std::vector<std::string> messages; // Queu of messages player has sent.
	int money;		// How much money player has left.
	int handvalue;	// Sum of current hand cards.
	int betvalue;	// How much player is betting on current round.
	bool bet_set;	// Has player succesfully set his bet.
	bool pass;		// Can player make any further moves.
	bool playing;

	unsigned ID; // Unique ID used to differentiate players.
	static unsigned ID_COUNTER; // Increment ID during constructor.

	int network_errors; // Amount of errors during networking.

	bool lost_connection; // Has player lost connection to server.

	SOCKET* ClientSocket;

	std::mutex message_lock;
};