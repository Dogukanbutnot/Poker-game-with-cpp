#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <ctime>
#include <map>

// --- TEMEL YAPILAR ---
enum Suit { SPADES, HEARTS, DIAMONDS, CLUBS };
enum Rank { TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };
enum HandRank { HIGH_CARD, ONE_PAIR, TWO_PAIR, THREE_OF_A_KIND, STRAIGHT, FLUSH, FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH, ROYAL_FLUSH };

struct Card {
    Rank rank;
    Suit suit;
    std::string toString() const {
        std::string ranks[] = {"", "", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
        std::string suits[] = {"♠", "♥", "♦", "♣"}; // DÜZELTME: Her takım farklı sembol
        return ranks[rank] + suits[suit];
    }
};

struct EvalResult {
    HandRank handType;
    std::vector<int> values;
    bool operator>(const EvalResult& other) const {
        if (handType != other.handType) return handType > other.handType;
        return values > other.values;
    }
    bool operator==(const EvalResult& other) const {
        return handType == other.handType && values == other.values;
    }
};

// --- YARDIMCI SINIFLAR ---
class HandEvaluator {
public:
    static bool checkFlush(const std::vector<Card>& hand) {
        for (size_t i = 1; i < hand.size(); ++i) 
            if (hand[i].suit != hand[0].suit) return false;
        return true;
    }

    // DÜZELTME: A-2-3-4-5 (wheel) desteği eklendi
    static bool checkStraight(const std::vector<Card>& hand) {
        // Normal straight kontrolü (A-K-Q-J-10, K-Q-J-10-9, vb.)
        bool isNormalStraight = true;
        for (size_t i = 0; i < hand.size() - 1; ++i) {
            if (hand[i].rank != hand[i+1].rank + 1) {
                isNormalStraight = false;
                break;
            }
        }
        if (isNormalStraight) return true;
        
        // A-2-3-4-5 (wheel/bicycle) kontrolü
        if (hand[0].rank == ACE && hand[1].rank == FIVE && 
            hand[2].rank == FOUR && hand[3].rank == THREE && 
            hand[4].rank == TWO) {
            return true;
        }
        
        return false;
    }

    static EvalResult evaluateFive(std::vector<Card> hand) {
        std::sort(hand.begin(), hand.end(), [](const Card& a, const Card& b) { 
            return a.rank > b.rank; 
        });
        
        bool isFlush = checkFlush(hand);
        bool isStraight = checkStraight(hand);
        
        std::map<Rank, int> counts;
        for (const auto& c : hand) counts[c.rank]++;
        
        std::vector<std::pair<Rank, int>> freq;
        for (auto const& [rank, count] : counts) 
            freq.push_back({rank, count});
        
        std::sort(freq.begin(), freq.end(), [](const auto& a, const auto& b) {
            return a.second != b.second ? a.second > b.second : a.first > b.first;
        });

        // DÜZELTME: Royal Flush kontrolü düzeltildi (10-J-Q-K-A olmalı)
        if (isFlush && isStraight) {
            bool isRoyal = (hand[0].rank == ACE && hand[4].rank == TEN);
            return {isRoyal ? ROYAL_FLUSH : STRAIGHT_FLUSH, {(int)hand[0].rank}};
        }
        
        if (freq[0].second == 4) 
            return {FOUR_OF_A_KIND, {(int)freq[0].first, (int)freq[1].first}};
        
        if (freq[0].second == 3 && freq[1].second == 2) 
            return {FULL_HOUSE, {(int)freq[0].first, (int)freq[1].first}};
        
        if (isFlush) 
            return {FLUSH, {(int)hand[0].rank, (int)hand[1].rank, (int)hand[2].rank, (int)hand[3].rank, (int)hand[4].rank}};
        
        if (isStraight) 
            return {STRAIGHT, {(int)hand[0].rank}};
        
        if (freq[0].second == 3) 
            return {THREE_OF_A_KIND, {(int)freq[0].first, (int)freq[1].first, (int)freq[2].first}};
        
        if (freq[0].second == 2 && freq[1].second == 2) 
            return {TWO_PAIR, {(int)freq[0].first, (int)freq[1].first, (int)freq[2].first}};
        
        if (freq[0].second == 2) 
            return {ONE_PAIR, {(int)freq[0].first, (int)freq[1].first, (int)freq[2].first, (int)freq[3].first}};
        
        return {HIGH_CARD, {(int)hand[0].rank, (int)hand[1].rank, (int)hand[2].rank, (int)hand[3].rank, (int)hand[4].rank}};
    }

    static EvalResult getBestFive(std::vector<Card> seven) {
        EvalResult best = {HIGH_CARD, {0}};
        std::string bitmask(5, 1); 
        bitmask.resize(7, 0);
        
        do {
            std::vector<Card> current;
            for(int i=0; i<7; ++i) 
                if(bitmask[i]) current.push_back(seven[i]);
            
            EvalResult res = evaluateFive(current);
            if (res > best) best = res;
        } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
        
        return best;
    }
};

class Deck {
    std::vector<Card> cards;
public:
    Deck() {
        for(int s=0; s<4; ++s) 
            for(int r=2; r<=14; ++r) 
                cards.push_back({(Rank)r, (Suit)s});
        
        // DÜZELTME: Daha iyi random seed
        std::random_device rd;
        std::default_random_engine rng(rd());
        std::shuffle(cards.begin(), cards.end(), rng);
    }
    Card draw() { 
        Card c = cards.back(); 
        cards.pop_back(); 
        return c; 
    }
};

// --- OYUN YÖNETİMİ ---
struct Player {
    std::string name;
    int chips;
    std::vector<Card> hand;
    int currentBet = 0;
    bool folded = false;
    bool isAI = false;
};

int main() {
    std::cout << "=== C++ TEXAS HOLD'EM POKER ===\n" << std::endl;
    
    Deck deck;
    std::vector<Player> players = { 
        {"Sen", 1000, {}, 0, false, false}, 
        {"AI_Bot", 1000, {}, 0, false, true} 
    };
    std::vector<Card> board;
    int pot = 0;
    int smallBlind = 10;
    int bigBlind = 20;

    // Blinds
    players[0].chips -= smallBlind;
    players[0].currentBet = smallBlind;
    players[1].chips -= bigBlind;
    players[1].currentBet = bigBlind;
    pot = smallBlind + bigBlind;
    
    std::cout << "Small Blind: " << smallBlind << " (Sen)\n";
    std::cout << "Big Blind: " << bigBlind << " (AI_Bot)\n";
    std::cout << "Pot: " << pot << "\n\n";

    // 1. Kart Dağıtımı
    for(auto& p : players) {
        p.hand = {deck.draw(), deck.draw()};
        if(!p.isAI) {
            std::cout << p.name << " Elin: " << p.hand[0].toString() 
                     << " " << p.hand[1].toString() << "\n";
        }
    }
    std::cout << "AI_Bot kartları gizli\n\n";

    // 2. Flop
    std::cout << "=== FLOP ===\n";
    for(int i=0; i<3; ++i) board.push_back(deck.draw());
    std::cout << "BOARD: ";
    for(auto& c : board) std::cout << c.toString() << " ";
    std::cout << "\n\n";

    // 3. Turn
    std::cout << "=== TURN ===\n";
    board.push_back(deck.draw());
    std::cout << "BOARD: ";
    for(auto& c : board) std::cout << c.toString() << " ";
    std::cout << "\n\n";

    // 4. River
    std::cout << "=== RIVER ===\n";
    board.push_back(deck.draw());
    std::cout << "BOARD: ";
    for(auto& c : board) std::cout << c.toString() << " ";
    std::cout << "\n\n";

    // 5. Kazananı Belirle
    std::cout << "=== SHOWDOWN ===\n";
    
    std::vector<std::pair<int, EvalResult>> results;
    std::string handTypes[] = {"Yuksek Kart", "Bir Cift", "Iki Cift", "Uclu", 
                               "Sira (Straight)", "Renk (Flush)", "Full House", 
                               "Kare", "Straight Flush", "Royal Flush"};

    for(size_t i = 0; i < players.size(); ++i) {
        auto& p = players[i];
        std::vector<Card> seven = p.hand;
        seven.insert(seven.end(), board.begin(), board.end());
        EvalResult res = HandEvaluator::getBestFive(seven);
        results.push_back({i, res});
        
        std::cout << p.name << " Kartlari: " << p.hand[0].toString() 
                 << " " << p.hand[1].toString() << "\n";
        std::cout << p.name << " El Değeri: " << handTypes[res.handType] << "\n\n";
    }

    // DÜZELTME: Beraberlik kontrolü eklendi
    EvalResult bestHand = results[0].second;
    std::vector<int> winners;
    winners.push_back(0);

    for(size_t i = 1; i < results.size(); ++i) {
        if (results[i].second > bestHand) {
            bestHand = results[i].second;
            winners.clear();
            winners.push_back(i);
        } else if (results[i].second == bestHand) {
            winners.push_back(i);
        }
    }

    std::cout << "=== SONUC ===\n";
    if (winners.size() == 1) {
        int winnerIdx = winners[0];
        std::cout << "KAZANAN: " << players[winnerIdx].name << "!\n";
        players[winnerIdx].chips += pot;
        std::cout << players[winnerIdx].name << " " << pot << " chip kazandi!\n";
    } else {
        std::cout << "BERABERE! Pot paylasildi:\n";
        int splitAmount = pot / winners.size();
        for(int idx : winners) {
            std::cout << "- " << players[idx].name << "\n";
            players[idx].chips += splitAmount;
        }
    }

    std::cout << "\n=== CHIP DURUMU ===\n";
    for(auto& p : players) {
        std::cout << p.name << ": " << p.chips << " chip\n";
    }

    return 0;
}
