#ifndef BOT_IMPL
#define BOT_IMPL

#include "bot.h"

std::string Bot::GetName() {
    return name;
}

void Bot::SetHand(const std::vector<Card>& cards) {
    hand = cards;
}

void Bot::RemoveCard(Card c) {
    auto it = std::find(ALL(hand), c);
    require(it != hand.end(), "Card played by " + name + " not in hand.");
    hand.erase(it);
}

std::vector<Card> Bot::GetHand() {
    return hand;
}

#endif // BOT_IMPL
