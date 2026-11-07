#include "ScoundrelCore.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace scoundrel {

namespace {

// A small, well-mixed PRNG. The core takes a seed and never reaches for a clock,
// so a dungeon is replayable from its seed.
struct Rng {
  uint32_t state;
  uint32_t next() {
    state += 0x6D2B79F5u;
    uint32_t t = state;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return t ^ (t >> 14);
  }
  int below(const int n) { return static_cast<int>(next() % static_cast<uint32_t>(n)); }
};

void buildDeck(uint8_t* deck) {
  int at = 0;
  for (int suit = 0; suit < 2; ++suit) {
    for (int value = 2; value <= 14; ++value) deck[at++] = makeCard(Kind::Monster, value, suit);
  }
  for (int value = 2; value <= 10; ++value) deck[at++] = makeCard(Kind::Weapon, value);
  for (int value = 2; value <= 10; ++value) deck[at++] = makeCard(Kind::Potion, value);
}

// Take `count` cards off the top of the deck.
void popTop(Game& g, const int count) {
  std::memmove(g.deck, g.deck + count, static_cast<size_t>(g.deckLen - count));
  g.deckLen = static_cast<uint8_t>(g.deckLen - count);
}

// Drops deck cards into the room's empty slots, in slot order.
void refillRoom(Game& g) {
  for (int slot = 0; slot < kRoomSize && g.deckLen > 0; ++slot) {
    if (g.room[slot] != kNoCard) continue;
    g.room[slot] = g.deck[0];
    popTop(g, 1);
  }
}

void checkEnd(Game& g) {
  if (g.over) return;
  if (g.hp <= 0) {
    g.over = true;
    g.won = false;
    g.score = static_cast<int16_t>(g.hp - remainingMonsterSum(g));
  } else if (g.deckLen == 0 && roomCount(g) == 0) {
    g.over = true;
    g.won = true;
    const int bonus = (g.hp == g.maxHp && g.lastAction == Action::Potion && g.lastPotion) ? g.lastPotion : 0;
    g.score = static_cast<int16_t>(g.hp + bonus);
  }
}

void afterResolve(Game& g) {
  g.resolved = static_cast<uint8_t>(g.resolved + 1);
  if (g.resolved == kResolvePerRoom) {
    const bool hadDeck = g.deckLen > 0;
    refillRoom(g);
    g.resolved = 0;
    g.potionUsed = false;
    g.fledLast = false;
    if (hadDeck) g.roomNo = static_cast<uint8_t>(g.roomNo + 1);
  }
  // The last one or two cards of the deck join the room at once.
  if (g.deckLen > 0 && g.deckLen < 3 && roomCount(g) < kRoomSize) refillRoom(g);
  checkEnd(g);
}

}  // namespace

const char* rankLabel(const uint8_t card) {
  static const char* kLabels[13] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
  if (card == kNoCard) return "";
  const int v = valueOf(card);
  return (v < 2 || v > 14) ? "?" : kLabels[v - 2];
}

const char* cardName(const uint8_t card) {
  if (card == kNoCard) return "";
  const int v = valueOf(card);
  switch (kindOf(card)) {
    case Kind::Monster:
      if (v == 11) return "JACK";
      if (v == 12) return "QUEEN";
      if (v == 13) return "KING";
      if (v == 14) return "ACE";
      if (v == 2) return "GOBLIN";
      if (v <= 4) return "WOLF";
      if (v <= 6) return "SKELETON";
      if (v <= 8) return "ORC";
      return "BEAST";
    case Kind::Weapon:
      if (v <= 3) return "DAGGER";
      if (v <= 5) return "SHORTSWORD";
      if (v <= 8) return "LONGSWORD";
      return "GREATSWORD";
    case Kind::Potion:
    default:
      if (v <= 3) return "SMALL POTION";
      if (v <= 8) return "POTION";
      return "LARGE POTION";
  }
}

int roomCount(const Game& g) {
  int n = 0;
  for (int slot = 0; slot < kRoomSize; ++slot) n += g.room[slot] != kNoCard;
  return n;
}

int monstersLeft(const Game& g) {
  int n = 0;
  for (int i = 0; i < g.deckLen; ++i) n += isMonster(g.deck[i]);
  for (int slot = 0; slot < kRoomSize; ++slot) n += isMonster(g.room[slot]);
  return n;
}

int remainingMonsterSum(const Game& g) {
  int sum = 0;
  for (int i = 0; i < g.deckLen; ++i) sum += isMonster(g.deck[i]) ? valueOf(g.deck[i]) : 0;
  for (int slot = 0; slot < kRoomSize; ++slot) sum += isMonster(g.room[slot]) ? valueOf(g.room[slot]) : 0;
  return sum;
}

void start(Game& g, const uint32_t seed, int maxHp) {
  if (maxHp < kMinHp || maxHp > kMaxHp) maxHp = kDefaultHp;
  std::memset(&g, 0, sizeof(g));
  uint8_t deck[kDeckSize];
  buildDeck(deck);
  Rng rng{seed};
  for (int i = kDeckSize - 1; i > 0; --i) {
    const int j = rng.below(i + 1);
    const uint8_t t = deck[i];
    deck[i] = deck[j];
    deck[j] = t;
  }
  for (int slot = 0; slot < kRoomSize; ++slot) g.room[slot] = deck[slot];
  std::memcpy(g.deck, deck + kRoomSize, kDeckSize - kRoomSize);
  g.deckLen = kDeckSize - kRoomSize;
  g.hp = static_cast<int8_t>(maxHp);
  g.maxHp = static_cast<int8_t>(maxHp);
  g.weapon = kNoCard;
  g.weaponLast = -1;
  g.roomNo = 1;
  g.note = Note::NewGame;
  g.noteCard = kNoCard;
}

bool weaponCanHit(const Game& g, const uint8_t monster) {
  if (g.weapon == kNoCard || !isMonster(monster)) return false;
  return g.weaponLast < 0 || valueOf(monster) <= g.weaponLast;
}

Preview preview(const Game& g, const uint8_t monster) {
  Preview p{valueOf(monster), -1};
  if (weaponCanHit(g, monster)) {
    const int d = valueOf(monster) - valueOf(g.weapon);
    p.weapon = d > 0 ? d : 0;
  }
  return p;
}

bool resolve(Game& g, const int slot, const Mode mode) {
  if (g.over || slot < 0 || slot >= kRoomSize) return false;
  const uint8_t card = g.room[slot];
  if (card == kNoCard) return false;
  g.started = true;
  g.noteCard = card;
  g.noteAmount = 0;
  const int value = valueOf(card);

  switch (kindOf(card)) {
    case Kind::Monster: {
      const bool useWeapon = mode != Mode::Fist && weaponCanHit(g, card);
      int damage;
      if (useWeapon) {
        damage = value - valueOf(g.weapon);
        if (damage < 0) damage = 0;
        g.weaponLast = static_cast<int8_t>(value);
        if (g.slainLen < kMaxSlain) g.slain[g.slainLen++] = card;
        g.note = Note::SlewWithWeapon;
      } else {
        damage = value;
        g.note = (mode == Mode::Weapon && g.weapon != kNoCard) ? Note::TooDull : Note::BareHands;
      }
      g.hp = static_cast<int8_t>(g.hp - damage);
      g.noteAmount = static_cast<int8_t>(damage);
      g.lastAction = Action::Fight;
      break;
    }
    case Kind::Weapon:
      g.weapon = card;
      g.weaponLast = -1;
      g.slainLen = 0;
      g.note = Note::Equipped;
      g.lastAction = Action::Equip;
      break;
    case Kind::Potion:
    default:
      if (g.potionUsed) {
        g.note = Note::Wasted;
        // engine.js leaves lastAction alone for a wasted potion.
      } else {
        const int before = g.hp;
        int after = g.hp + value;
        if (after > g.maxHp) after = g.maxHp;
        g.hp = static_cast<int8_t>(after);
        g.noteAmount = static_cast<int8_t>(after - before);
        g.potionUsed = true;
        g.lastAction = Action::Potion;
        g.lastPotion = static_cast<uint8_t>(value);
        g.note = Note::Drank;
      }
      break;
  }

  g.room[slot] = kNoCard;
  afterResolve(g);
  return true;
}

bool canFlee(const Game& g) { return !g.over && roomCount(g) == kRoomSize && !g.fledLast; }

bool flee(Game& g) {
  if (!canFlee(g)) {
    if (!g.over && g.fledLast) {
      g.note = Note::CannotFlee;
      g.noteCard = kNoCard;
    }
    return false;
  }
  // The room goes under the deck, in slot order, and the next four come up.
  uint8_t all[kDeckSize];
  int n = 0;
  for (int i = 0; i < g.deckLen; ++i) all[n++] = g.deck[i];
  for (int slot = 0; slot < kRoomSize; ++slot) all[n++] = g.room[slot];
  for (int slot = 0; slot < kRoomSize; ++slot) g.room[slot] = all[slot];
  std::memcpy(g.deck, all + kRoomSize, static_cast<size_t>(n - kRoomSize));
  g.deckLen = static_cast<uint8_t>(n - kRoomSize);
  g.resolved = 0;
  g.potionUsed = false;
  g.fledLast = true;
  g.roomNo = static_cast<uint8_t>(g.roomNo + 1);
  g.lastAction = Action::Flee;
  g.note = Note::Fled;
  g.noteCard = kNoCard;
  return true;
}

// Format, version 1:
//   1 deckLen deck... room0..3 hp maxHp weapon weaponLast slainLen slain...
//   resolved potionUsed fledLast started roomNo over won score lastAction
//   lastPotion note noteCard noteAmount
int serialize(const Game& g, char* buffer, const int size) {
  int used = 0;
  auto put = [&](const int v) {
    if (used < 0 || used >= size) {
      used = -1;
      return;
    }
    const int w = std::snprintf(buffer + used, static_cast<size_t>(size - used), used ? " %d" : "%d", v);
    used = (w < 0 || used + w >= size) ? -1 : used + w;
  };
  put(1);
  put(g.deckLen);
  for (int i = 0; i < g.deckLen; ++i) put(g.deck[i]);
  for (int slot = 0; slot < kRoomSize; ++slot) put(g.room[slot]);
  put(g.hp);
  put(g.maxHp);
  put(g.weapon);
  put(g.weaponLast);
  put(g.slainLen);
  for (int i = 0; i < g.slainLen; ++i) put(g.slain[i]);
  put(g.resolved);
  put(g.potionUsed);
  put(g.fledLast);
  put(g.started);
  put(g.roomNo);
  put(g.over);
  put(g.won);
  put(g.score);
  put(static_cast<int>(g.lastAction));
  put(g.lastPotion);
  put(static_cast<int>(g.note));
  put(g.noteCard);
  put(g.noteAmount);
  return used;
}

namespace {

bool validCard(const long v, const bool allowNone) {
  if (v == kNoCard) return allowNone;
  if (v < 0 || v > 0x7F) return false;
  const uint8_t c = static_cast<uint8_t>(v);
  const int value = valueOf(c);
  switch (kindOf(c)) {
    case Kind::Monster:
      return value >= 2 && value <= 14;
    case Kind::Weapon:
    case Kind::Potion:
      return suitOf(c) == 0 && value >= 2 && value <= 10;
    default:
      return false;
  }
}

}  // namespace

bool parse(const char* text, Game& out) {
  if (!text) return false;
  const char* cursor = text;
  bool ok = true;
  auto take = [&](const long lo, const long hi) -> long {
    if (!ok) return 0;
    char* next = nullptr;
    const long v = std::strtol(cursor, &next, 10);
    if (next == cursor || v < lo || v > hi) {
      ok = false;
      return 0;
    }
    cursor = next;
    return v;
  };

  Game g;
  std::memset(&g, 0, sizeof(g));
  if (take(1, 1) != 1) return false;
  g.deckLen = static_cast<uint8_t>(take(0, kDeckSize));
  for (int i = 0; ok && i < g.deckLen; ++i) {
    const long c = take(0, 255);
    if (!validCard(c, false)) ok = false;
    g.deck[i] = static_cast<uint8_t>(c);
  }
  for (int slot = 0; ok && slot < kRoomSize; ++slot) {
    const long c = take(0, 255);
    if (!validCard(c, true)) ok = false;
    g.room[slot] = static_cast<uint8_t>(c);
  }
  g.hp = static_cast<int8_t>(take(-100, kMaxHp));
  g.maxHp = static_cast<int8_t>(take(kMinHp, kMaxHp));
  {
    const long c = take(0, 255);
    if (!validCard(c, true) || (c != kNoCard && kindOf(static_cast<uint8_t>(c)) != Kind::Weapon)) ok = false;
    g.weapon = static_cast<uint8_t>(c);
  }
  g.weaponLast = static_cast<int8_t>(take(-1, 14));
  g.slainLen = static_cast<uint8_t>(take(0, kMaxSlain));
  for (int i = 0; ok && i < g.slainLen; ++i) {
    const long c = take(0, 255);
    if (!validCard(c, false) || !isMonster(static_cast<uint8_t>(c))) ok = false;
    g.slain[i] = static_cast<uint8_t>(c);
  }
  g.resolved = static_cast<uint8_t>(take(0, kResolvePerRoom - 1));
  g.potionUsed = take(0, 1) != 0;
  g.fledLast = take(0, 1) != 0;
  g.started = take(0, 1) != 0;
  g.roomNo = static_cast<uint8_t>(take(1, 255));
  g.over = take(0, 1) != 0;
  g.won = take(0, 1) != 0;
  g.score = static_cast<int16_t>(take(-400, 400));
  g.lastAction = static_cast<Action>(take(0, static_cast<long>(Action::Flee)));
  g.lastPotion = static_cast<uint8_t>(take(0, 10));
  g.note = static_cast<Note>(take(0, static_cast<long>(Note::CannotFlee)));
  g.noteCard = static_cast<uint8_t>(take(0, 255));
  g.noteAmount = static_cast<int8_t>(take(-100, 100));
  if (!ok) return false;

  // Every card exactly once across deck, room, the weapon, its kills and the
  // discard is not checkable (the discard is not stored), but no card may
  // appear twice among the ones that are.
  bool seen[128] = {};
  auto mark = [&](const uint8_t c) {
    if (c == kNoCard) return true;
    if (seen[c & 0x7F]) return false;
    seen[c & 0x7F] = true;
    return true;
  };
  for (int i = 0; i < g.deckLen; ++i) ok = ok && mark(g.deck[i]);
  for (int slot = 0; slot < kRoomSize; ++slot) ok = ok && mark(g.room[slot]);
  ok = ok && mark(g.weapon);
  for (int i = 0; i < g.slainLen; ++i) ok = ok && mark(g.slain[i]);
  if (!ok) return false;

  out = g;
  return true;
}

}  // namespace scoundrel
