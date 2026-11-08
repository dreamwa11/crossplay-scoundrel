#pragma once

// Scoundrel's rules. Freestanding C++17: no renderer, no storage, no clock.
//
// A port of the house rules of Fotis's own web version (engine.js), kept
// behaviour-for-behaviour so a game plays the same on the panel as on the phone:
//
//  - 44 cards. Clubs and spades 2..A are monsters (J=11 Q=12 K=13 A=14),
//    diamonds 2..10 are weapons, hearts 2..10 are potions.
//  - A room holds 4 cards. After 3 are resolved it is refilled to 4.
//  - Flee only from a full room, never twice in a row. The room goes to the
//    bottom of the deck.
//  - One potion per room; a second one is discarded with no effect.
//  - A weapon can hit a monster only if it has no kills yet, or the monster is
//    no stronger than its last kill.
//  - When 1-2 cards remain in the deck they join the room straight away.
//  - Score: died -> HP minus the remaining monsters' values. Survived -> HP,
//    plus the last potion's value if HP is full and drinking it was the last act.
//
// The room is FOUR FIXED SLOTS rather than a list. A card stays where it was
// dealt while its neighbours are resolved, and a refill drops new cards into
// the holes. On a panel that repaints in half a second, a card that slides
// sideways under your finger is a card you tap by mistake.

#include <cstdint>

namespace scoundrel {

enum class Kind : uint8_t { Monster = 0, Weapon = 1, Potion = 2 };

// One byte per card: bits 0-3 value-2 (0..12), bits 4-5 kind, bit 6 suit
// (clubs 0 / spades 1, monsters only). 0xFF is an empty slot.
constexpr uint8_t kNoCard = 0xFF;
constexpr int kDeckSize = 44;
constexpr int kRoomSize = 4;
constexpr int kResolvePerRoom = 3;
constexpr int kDefaultHp = 20;
constexpr int kMinHp = 10;
constexpr int kMaxHp = 30;
// A weapon can slay at most every monster in the deck.
constexpr int kMaxSlain = 26;

inline uint8_t makeCard(const Kind kind, const int value, const int suit = 0) {
  return static_cast<uint8_t>(((suit & 1) << 6) | (static_cast<int>(kind) << 4) | ((value - 2) & 0x0F));
}
inline int valueOf(const uint8_t card) { return (card & 0x0F) + 2; }
inline Kind kindOf(const uint8_t card) { return static_cast<Kind>((card >> 4) & 0x03); }
inline int suitOf(const uint8_t card) { return (card >> 6) & 1; }
inline bool isMonster(const uint8_t card) { return card != kNoCard && kindOf(card) == Kind::Monster; }

// The corner index: 2..10, J, Q, K, A. Static storage.
const char* rankLabel(uint8_t card);
// "Goblin", "Longsword", "Small potion": the phone version's names, sentence case.
const char* cardName(uint8_t card);

// What a tap on a monster means.
enum class Mode : uint8_t {
  Auto = 0,  // the weapon if it can hit, else bare hands
  Weapon,
  Fist,
};

enum class Action : uint8_t { None = 0, Fight, Potion, Equip, Wasted, Flee };

// What just happened, for the message line. The screens turn it into words;
// the core never formats text.
enum class Note : uint8_t {
  None = 0,
  NewGame,
  SlewWithWeapon,  // amount = damage taken (may be 0)
  TooDull,         // asked for the weapon, it could not hit: bare hands, amount = damage
  BareHands,       // amount = damage
  Equipped,
  Drank,       // amount = HP healed (0 = already full)
  Wasted,      // a second potion in one room
  Fled,
  CannotFlee,  // twice in a row
};

struct Game {
  uint8_t deck[kDeckSize];
  uint8_t deckLen;
  uint8_t room[kRoomSize];  // kNoCard where a card has been resolved
  int8_t hp;
  int8_t maxHp;
  uint8_t weapon;     // kNoCard when bare-handed
  int8_t weaponLast;  // value of the last monster it slew, or -1 for none yet
  uint8_t slain[kMaxSlain];
  uint8_t slainLen;
  uint8_t resolved;  // cards resolved in the current room
  bool potionUsed;   // a potion was drunk in this room
  bool fledLast;     // the current room was entered by fleeing
  bool started;      // any card has been resolved this dungeon
  uint8_t roomNo;
  bool over;
  bool won;
  int16_t score;
  Action lastAction;
  uint8_t lastPotion;  // value, 0 for none
  Note note;
  uint8_t noteCard;
  int8_t noteAmount;
};

int roomCount(const Game& game);
int monstersLeft(const Game& game);  // in deck and room
int remainingMonsterSum(const Game& game);

// Shuffles a fresh deck from `seed` and deals the first room.
void start(Game& game, uint32_t seed, int maxHp = kDefaultHp);

bool weaponCanHit(const Game& game, uint8_t monster);

// Damage previews for a monster: bare hands, and the weapon (or -1 when the
// weapon cannot hit it, or there is none).
struct Preview {
  int fist;
  int weapon;
};
Preview preview(const Game& game, uint8_t monster);

// Resolve the card in room slot `slot`. Returns false (and changes nothing)
// for an empty slot or a finished game.
bool resolve(Game& game, int slot, Mode mode = Mode::Auto);

bool canFlee(const Game& game);
// Returns false when fleeing is not allowed; sets Note::CannotFlee when that is
// because the last room was fled.
bool flee(Game& game);

// Text round trip for the save file: space-separated integers, no line breaks.
// `parse` validates every field and leaves `out` untouched on failure.
int serialize(const Game& game, char* buffer, int size);
bool parse(const char* text, Game& out);

}  // namespace scoundrel
