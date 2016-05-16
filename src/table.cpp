#include "table.h"
#include <algorithm>
#include <random>
#include <chrono>

unsigned int Table::TABLE_SIZE = 0;

Table::Table() {}

void Table::Start() {

  std::cout << "\n\nA new game has started!\n\n";

  for(size_t i = 0; i < players.size(); ++i)
    players[i].sendIndex(i);

  // choose players in play

  Card::lowestCard = 15 - players.size() * 2;

  score.resize(TABLE_SIZE, 0);

  // true if a game was already played but a certain connection, false otherwise
  bool gamesPlayed[players.size()][gamesNumber+1];

  for(size_t i=0;i<players.size();++i)
    for(int j=0;j<gamesNumber+1;++j)
      gamesPlayed[i][j] = false;

  // assign the first connection
  firstPlayer = players.begin();

  for(int gameChoice = 1; gameChoice <= gamesNumber; ++gameChoice) {
    
    for(size_t i=0;i<players.size();++i) {
      // set the first player
      firstPlayer = players.begin() + i;
      
      std::cout << "Pre-round:\n";
      std::cout << "First player is " << firstPlayer->getName() << '\n';

      // check if connection wants to play NV mode and enforce it for the last choice
      if(gameChoice < gamesNumber) {
        modeNV = players[i].getNVChoice();
      } else {
        modeNV = true;
      }

      if(modeNV) {
        std::cout << "NV Mode chosen!\n";
      } else {
        std::cout << "NV Mode not chosen!\n";
      }

      // if NV mode not chosen, give cards before game choice
      if(modeNV == false) {
        GiveCards();
        gameType = players[i].getGameChoice();
      } else {
        gameType = players[i].getGameChoice();
        GiveCards();
      }

      // check if connection can play that game
      require(gameType >= 1 && gameType <= gamesNumber,
          players[i].getName() + " has chosen a game index out of bounds\n");
      
      std::cout << players[i].getName() << " chose to play: " << GameName[gameType] << '\n';

      require(gamesPlayed[i][gameType] == false,
          players[i].getName() + " had already chosen that game\n");

      gamesPlayed[i][gameType] = true;

      // let the other players know the game type
      for(auto &connection : players)
        connection.sendGameChoice(static_cast<uint8_t>(gameType));

      // start the round
      PlayRound();
    }
  }
  std::cout << "Game is over!\n";
  printScores();

  for(auto &connection: players)
    connection.gameEnd();
}

void Table::PlayerAction(Connection &connection) {
  // send connection the cards that were played and get his played card
  connection.sendCards(cardStack);

  Card playedCard = connection.getCardChoice();

  std::cout << connection.getName() << " played " << playedCard << "\n";

  // check if the played card can be played
  if(cardStack.size() > 1) {
    if(!playedCard.isSameSuite(cardStack.front())) {
      auto cards = connection.getHand();
      for(auto c : cards)
        require(!cardStack.front().isSameSuite(c),
            connection.getName() + " had chosen a card of a different suite.\n");
    }
  }

  // push the card in play
  cardStack.push_back(playedCard);
}

void Table::IterateThroughPlayers(std::vector<Connection>::iterator iterator) {
  for(auto it = iterator; it != players.end(); ++it)
    PlayerAction(*it);
  for(auto it = players.begin(); it != iterator; ++it)
    PlayerAction(*it);
  std::cout << std::endl;
}

void Table::GiveCards() {
  // get all cards in play
  std::vector<Card> allCards = Card::getAllCards();

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(allCards.begin(), allCards.end(), std::default_random_engine(seed));

  auto card = allCards.begin();

  // give cards to players
  for(size_t i=0;i<players.size();++i) {
    std::vector<Card> hand;
    for(int j=1;j<=8;++j) {
      hand.push_back(*card);
      ++card;
    }
    players[i].sendHand(hand);
  }
}

void Table::printScores() {
  std::cout << "Scores:\n";
  for(size_t i=0;i<players.size();++i) {
    players[i].sendScores(score);
    std::cout << players[i].getName() + ": " << score[i] << "\n";
  }
}

void Table::PlayRound() {
  std::cout << "\nStarting new round!\n\n";

  for(int roundstep = 1; roundstep <= 8; ++roundstep) {
    cardStack.clear();

    // get players to play cards
    IterateThroughPlayers(firstPlayer);

    // announce all players about the cards that were played
    for(size_t i=0;i<players.size();++i)
      players[i].sendCards(cardStack);

    // check for winner
    int winnerIndex = 0;
    Card winnerCard = cardStack[winnerIndex];

    for(size_t i=1;i<cardStack.size();++i) {
      if(winnerCard.isBeatenBy(cardStack[i])) {
        winnerCard = cardStack[i];
        winnerIndex = i;
      }
    }

    // change the first connection to go
    while(winnerIndex--) {
      ++firstPlayer;
      if(firstPlayer == players.end())
        firstPlayer = players.begin();
    }

    // change score
    ChangeScore();
  }

  std::cout << "Round over!\n\n";

  // let players know the scores
  printScores();
  std::cout << "\n--------------------------\n";

  std::cout << std::endl;
}

Table::~Table() {
  players.clear();
}

void Table::addPlayer(int fd) {
  players.emplace_back(Connection(fd));
  if(players.size() == TABLE_SIZE)
    Start();
}
