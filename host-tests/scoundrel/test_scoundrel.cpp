// Scoundrel rules tests. No device, no PlatformIO: ScoundrelCore is freestanding.
//
//   host-tests/scoundrel/run.sh
//
// `test_scoundrel --trace N` prints N random games as action traces instead, for
// replaying against another implementation of the same rules.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ScoundrelCore.h"

namespace s = scoundrel;

static int failures = 0;
static int checks = 0;
#define CHECK(cond)                                                    \
  do {                                                                 \
    ++checks;                                                          \
    if (!(cond)) {                                                     \
      ++failures;                                                      \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
    }                                                                  \
  } while (0)

// A game with a known room and deck. Cards are given as (kind, value).
static s::Game rigged(const uint8_t room[4], const uint8_t* deck, const int deckLen, const int hp = 20) {
  s::Game g;
  s::start(g, 1, 20);
  for (int i = 0; i < 4; ++i) g.room[i] = room[i];
  std::memcpy(g.deck, deck, static_cast<size_t>(deckLen));
  g.deckLen = static_cast<uint8_t>(deckLen);
  g.hp = static_cast<int8_t>(hp);
  return g;
}

static uint8_t M(const int v, const int suit = 0) { return s::makeCard(s::Kind::Monster, v, suit); }
static uint8_t W(const int v) { return s::makeCard(s::Kind::Weapon, v); }
static uint8_t P(const int v) { return s::makeCard(s::Kind::Potion, v); }

static void testDeck() {
  s::Game g;
  s::start(g, 12345);
  CHECK(g.deckLen == 40);
  CHECK(s::roomCount(g) == 4);
  CHECK(g.hp == 20 && g.maxHp == 20);
  int monsters = 0, weapons = 0, potions = 0, monsterSum = 0;
  bool seen[128] = {};
  auto count = [&](const uint8_t c) {
    CHECK(!seen[c]);
    seen[c] = true;
    if (s::kindOf(c) == s::Kind::Monster) {
      ++monsters;
      monsterSum += s::valueOf(c);
    } else if (s::kindOf(c) == s::Kind::Weapon) {
      ++weapons;
      CHECK(s::valueOf(c) <= 10);
    } else {
      ++potions;
      CHECK(s::valueOf(c) <= 10);
    }
  };
  for (int i = 0; i < g.deckLen; ++i) count(g.deck[i]);
  for (int i = 0; i < 4; ++i) count(g.room[i]);
  CHECK(monsters == 26 && weapons == 9 && potions == 9);
  CHECK(monsterSum == 2 * (2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14));
  CHECK(s::remainingMonsterSum(g) == monsterSum);

  s::Game a, b;
  s::start(a, 7);
  s::start(b, 7);
  CHECK(std::memcmp(a.deck, b.deck, sizeof(a.deck)) == 0);
  s::start(b, 8);
  CHECK(std::memcmp(a.deck, b.deck, sizeof(a.deck)) != 0 || std::memcmp(a.room, b.room, 4) != 0);

  s::start(a, 1, 99);
  CHECK(a.maxHp == 20);
  s::start(a, 1, 25);
  CHECK(a.maxHp == 25 && a.hp == 25);
}

static void testNames() {
  CHECK(std::strcmp(s::cardName(M(2)), "Goblin") == 0);
  CHECK(std::strcmp(s::cardName(M(4)), "Wolf") == 0);
  CHECK(std::strcmp(s::cardName(M(6)), "Skeleton") == 0);
  CHECK(std::strcmp(s::cardName(M(8)), "Orc") == 0);
  CHECK(std::strcmp(s::cardName(M(10, 1)), "Beast") == 0);
  CHECK(std::strcmp(s::cardName(M(11)), "Jack") == 0);
  CHECK(std::strcmp(s::cardName(M(14)), "Ace") == 0);
  CHECK(std::strcmp(s::cardName(W(3)), "Dagger") == 0);
  CHECK(std::strcmp(s::cardName(W(5)), "Shortsword") == 0);
  CHECK(std::strcmp(s::cardName(W(9)), "Greatsword") == 0);
  CHECK(std::strcmp(s::cardName(P(3)), "Small potion") == 0);
  CHECK(std::strcmp(s::cardName(P(10)), "Large potion") == 0);
  CHECK(std::strcmp(s::rankLabel(M(14)), "A") == 0);
  CHECK(std::strcmp(s::rankLabel(W(10)), "10") == 0);
}

static void testFighting() {
  const uint8_t deck[] = {M(3), M(4), M(5), M(6), M(7), M(8)};
  const uint8_t room[] = {W(5), M(9), M(10), M(4)};
  s::Game g = rigged(room, deck, 6);

  // Bare hands: full damage.
  CHECK(s::resolve(g, 3));
  CHECK(g.hp == 16);
  CHECK(g.note == s::Note::BareHands && g.noteAmount == 4);
  CHECK(g.room[3] == s::kNoCard);

  // Equip, then the weapon takes the monster's value off.
  CHECK(s::resolve(g, 0));
  CHECK(g.weapon == W(5) && g.weaponLast == -1);
  // That was the third card of the room? No: two resolved so far.
  CHECK(g.resolved == 2);
  const s::Preview p = s::preview(g, M(9));
  CHECK(p.fist == 9 && p.weapon == 4);
  CHECK(s::resolve(g, 1));  // auto = weapon
  CHECK(g.hp == 12);
  CHECK(g.weaponLast == 9);
  CHECK(g.slainLen == 1 && g.slain[0] == M(9));
  // Third card resolved: the room refilled in its holes, slot 2 untouched.
  CHECK(g.resolved == 0);
  CHECK(g.room[2] == M(10));
  CHECK(g.room[0] == M(3) && g.room[1] == M(4) && g.room[3] == M(5));
  CHECK(g.roomNo == 2);
  CHECK(g.deckLen == 3);

  // The weapon cannot hit a 10 after a 9.
  CHECK(!s::weaponCanHit(g, M(10)));
  CHECK(s::preview(g, M(10)).weapon == -1);
  // Asking for it anyway falls back to bare hands and says so.
  CHECK(s::resolve(g, 2, s::Mode::Weapon));
  CHECK(g.hp == 2);
  CHECK(g.note == s::Note::TooDull);
  CHECK(g.weaponLast == 9);

  // Fist keeps the weapon's record untouched.
  CHECK(s::resolve(g, 0, s::Mode::Fist));
  CHECK(g.hp == -1);
  CHECK(g.over && !g.won);
  // Died: HP minus the monsters still out there.
  CHECK(g.score == -1 - s::remainingMonsterSum(g));
  CHECK(!s::resolve(g, 1));
}

static void testWeaponDegrades() {
  const uint8_t deck[] = {M(2), M(2, 1), M(3), M(3, 1), M(4), M(4, 1)};
  const uint8_t room[] = {W(10), M(6), M(7), M(8)};
  s::Game g = rigged(room, deck, 6);
  s::resolve(g, 0);
  s::resolve(g, 1);  // 6 first: now only <= 6
  CHECK(g.weaponLast == 6);
  CHECK(!s::weaponCanHit(g, M(7)));
  CHECK(s::weaponCanHit(g, M(6, 1)));
  CHECK(s::weaponCanHit(g, M(2)));
  // Damage is never negative.
  CHECK(s::preview(g, M(2)).weapon == 0);
  // A new weapon resets the record and drops the old kills.
  g.room[2] = W(3);
  s::resolve(g, 2);
  CHECK(g.weapon == W(3) && g.weaponLast == -1 && g.slainLen == 0);
}

static void testPotions() {
  const uint8_t deck[] = {M(2), M(3), M(4), M(5), M(6), M(7)};
  const uint8_t room[] = {P(5), P(9), M(10), M(2, 1)};
  s::Game g = rigged(room, deck, 6, 12);
  CHECK(s::resolve(g, 2));  // 12 - 10 = 2
  CHECK(g.hp == 2);
  CHECK(s::resolve(g, 1));  // +9
  CHECK(g.hp == 11 && g.note == s::Note::Drank && g.noteAmount == 9);
  CHECK(g.potionUsed);
  // The second potion of a room is wasted.
  CHECK(s::resolve(g, 0));
  CHECK(g.hp == 11 && g.note == s::Note::Wasted);
  CHECK(g.lastAction == s::Action::Potion);
  // ...and the new room allows one again.
  CHECK(!g.potionUsed);

  // Healing caps at max HP.
  const uint8_t room2[] = {P(10), M(2), M(3), M(4)};
  s::Game h = rigged(room2, deck, 6, 18);
  s::resolve(h, 0);
  CHECK(h.hp == 20 && h.noteAmount == 2);
}

static void testFlee() {
  const uint8_t deck[] = {M(2), M(3), M(4), M(5), M(6), M(7), M(8), M(9)};
  const uint8_t room[] = {M(14), M(13), M(12), M(11)};
  s::Game g = rigged(room, deck, 8);
  CHECK(s::canFlee(g));
  CHECK(s::flee(g));
  CHECK(g.room[0] == M(2) && g.room[3] == M(5));
  CHECK(g.deckLen == 8);
  CHECK(g.deck[4] == M(14) && g.deck[7] == M(11));
  CHECK(g.roomNo == 2 && g.fledLast);
  // Not twice in a row.
  CHECK(!s::canFlee(g));
  CHECK(!s::flee(g));
  CHECK(g.note == s::Note::CannotFlee);
  // Not from a partial room either.
  s::resolve(g, 0);
  CHECK(!s::canFlee(g));
  s::resolve(g, 1);
  s::resolve(g, 2);  // new room: flee allowed again
  CHECK(!g.fledLast);
  CHECK(s::canFlee(g));
}

static void testLastCardsAndWinning() {
  // Two cards left in the deck join the room after the next resolution.
  const uint8_t deck[] = {P(2), M(2)};
  const uint8_t room[] = {M(3), P(4), W(2), M(4)};
  s::Game g = rigged(room, deck, 2, 20);
  s::resolve(g, 0);  // 17, and one card is pulled straight into the hole
  CHECK(g.hp == 17);
  CHECK(g.deckLen == 1);
  CHECK(s::roomCount(g) == 4);
  CHECK(g.room[0] == P(2));
  s::resolve(g, 2);  // equip 2; the very last card is pulled into its slot
  CHECK(g.deckLen == 0);
  CHECK(g.room[2] == M(2));
  s::resolve(g, 3);  // 4 - 2 = 2 -> 15; third card, empty deck: no new room number
  CHECK(g.hp == 15);
  CHECK(g.roomNo == 1);
  s::resolve(g, 1);  // +4 -> 19
  CHECK(g.hp == 19);
  s::resolve(g, 0);  // a second potion in the same room: wasted
  CHECK(g.hp == 19 && g.note == s::Note::Wasted);
  CHECK(!g.over);
  s::resolve(g, 2);  // the last monster, with the weapon: 2 vs 2, no damage
  CHECK(g.over && g.won);
  CHECK(g.score == 19);  // last act was a fight: no potion bonus

  // The bonus: full HP and the last act was drinking.
  const uint8_t room2[] = {P(6), s::kNoCard, s::kNoCard, s::kNoCard};
  s::Game h = rigged(room2, nullptr, 0, 20);
  s::resolve(h, 0);
  CHECK(h.over && h.won && h.score == 26);
}

static void testSaveRoundTrip() {
  for (uint32_t seed = 1; seed < 200; ++seed) {
    s::Game g;
    s::start(g, seed, 20 + static_cast<int>(seed % 5));
    uint32_t r = seed * 2654435761u;
    for (int step = 0; step < static_cast<int>(seed % 30) && !g.over; ++step) {
      r = r * 1103515245u + 12345u;
      if ((r >> 8) % 9 == 0) {
        s::flee(g);
        continue;
      }
      s::resolve(g, static_cast<int>((r >> 16) % 4), static_cast<s::Mode>((r >> 4) % 3));
    }
    char buffer[512];
    const int n = s::serialize(g, buffer, sizeof(buffer));
    CHECK(n > 0);
    s::Game back;
    CHECK(s::parse(buffer, back));
    char again[512];
    CHECK(s::serialize(back, again, sizeof(again)) == n);
    CHECK(std::strcmp(buffer, again) == 0);
  }
  s::Game junk;
  CHECK(!s::parse("", junk));
  CHECK(!s::parse("2 0", junk));
  CHECK(!s::parse("1 40 1 2 3", junk));
  // A duplicated card is refused.
  s::Game g;
  s::start(g, 3);
  g.room[1] = g.room[0];
  char buffer[512];
  s::serialize(g, buffer, sizeof(buffer));
  CHECK(!s::parse(buffer, junk));
  // A buffer too small reports failure rather than a truncated line.
  CHECK(s::serialize(g, buffer, 20) < 0);
}

// Many random games: invariants that must hold at every step.
static void testSoak() {
  for (uint32_t seed = 1; seed <= 20000; ++seed) {
    s::Game g;
    s::start(g, seed);
    uint32_t r = seed;
    int steps = 0;
    while (!g.over && steps < 200) {
      ++steps;
      r = r * 1664525u + 1013904223u;
      const int before = g.deckLen + s::roomCount(g);
      if ((r >> 24) % 7 == 0 && s::canFlee(g)) {
        s::flee(g);
        CHECK(g.deckLen + s::roomCount(g) == before);
        continue;
      }
      int slot = static_cast<int>((r >> 16) % 4);
      while (g.room[slot] == s::kNoCard) slot = (slot + 1) % 4;
      const s::Preview p = s::isMonster(g.room[slot]) ? s::preview(g, g.room[slot]) : s::Preview{0, -1};
      const int hp = g.hp;
      const s::Mode mode = static_cast<s::Mode>((r >> 8) % 3);
      const bool monster = s::isMonster(g.room[slot]);
      CHECK(s::resolve(g, slot, mode));
      CHECK(g.deckLen + s::roomCount(g) == before - 1);
      if (monster) {
        const int expected = (mode != s::Mode::Fist && p.weapon >= 0) ? p.weapon : p.fist;
        CHECK(hp - g.hp == expected);
      }
      CHECK(g.hp <= g.maxHp);
      CHECK(g.resolved < 3);
      // A room is never empty while cards remain.
      if (!g.over && g.deckLen > 0) CHECK(s::roomCount(g) > 0);
    }
    CHECK(g.over);
    if (g.won) {
      CHECK(g.hp > 0 && g.score >= g.hp);
    } else {
      CHECK(g.hp <= 0 && g.score <= g.hp);
    }
  }
}

// Random games printed as traces, for replaying against engine.js.
static void trace(const int games) {
  for (int n = 1; n <= games; ++n) {
    s::Game g;
    s::start(g, static_cast<uint32_t>(n) * 7919u);
    std::printf("G");
    for (int i = 0; i < 4; ++i) std::printf(" %d", g.room[i]);
    for (int i = 0; i < g.deckLen; ++i) std::printf(" %d", g.deck[i]);
    std::printf("\n");
    uint32_t r = static_cast<uint32_t>(n);
    while (!g.over) {
      r = r * 1664525u + 1013904223u;
      int slot = static_cast<int>((r >> 16) % 4);
      while (g.room[slot] == s::kNoCard) slot = (slot + 1) % 4;
      const uint8_t card = g.room[slot];
      const int mode = static_cast<int>((r >> 8) % 3);
      s::resolve(g, slot, static_cast<s::Mode>(mode));
      std::printf("A %d %d %d %d %d %d %d %d %d %d\n", card, mode, g.hp, g.weapon == s::kNoCard ? -1 : g.weapon,
                  g.weaponLast, g.deckLen, s::roomCount(g), g.roomNo, g.over ? 1 : 0, g.over ? g.score : 0);
    }
  }
}

int main(int argc, char** argv) {
  if (argc == 3 && std::strcmp(argv[1], "--trace") == 0) {
    trace(std::atoi(argv[2]));
    return 0;
  }
  testDeck();
  testNames();
  testFighting();
  testWeaponDegrades();
  testPotions();
  testFlee();
  testLastCardsAndWinning();
  testSaveRoundTrip();
  testSoak();
  std::printf("scoundrel: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
