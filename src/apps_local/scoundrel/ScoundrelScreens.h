#pragma once

// Scoundrel on screen. Freestanding builders over plain models: FreeInkUI and
// Toybox tokens, nothing else, so host-tests/ui can build every screen against a
// fake target.
//
// The cards are plain white faces with one black glyph in the middle, the rank
// in the corner and the card's kind as a small mark opposite it. No painted art:
// the panel has four greys and refreshes in half a second, and a silhouette is
// the one picture that survives both.

#include "../ui/ToyboxScreen.h"
#include "ScoundrelCore.h"

namespace scoundrelui {

namespace fui = freeink::ui;

enum : fui::ActionId {
  ActionMenuRow = 1,
  ActionHowToNext = 2,
  // value = slot * 4 + Mode
  ActionCard = 3,
  ActionFlee = 4,
  ActionSeeResult = 5,
  ActionAgain = 6,
  ActionDone = 7,
};

enum class Screen : uint8_t { Menu = 0, HowTo, Board, Result };

enum class MenuRow : int { Continue = 0, NewGame, HowTo, Health, Count };

struct MenuModel {
  bool canContinue = false;
  int maxHp = scoundrel::kDefaultHp;
  int played = 0;
  int survived = 0;
  bool hasBest = false;
  int best = 0;
};

struct HowToModel {
  int page = 0;
};

struct BoardModel {
  scoundrel::Game game{};
};

struct ResultModel {
  scoundrel::Game game{};
  bool newBest = false;
  int best = 0;
};

// The rect of a room slot, and its inverse. Card bodies are hit-tested from the
// same geometry that drew them; the chips under them are ordinary buttons.
fui::Rect slotRect(const fui::DeviceContext& device, int slot);
int slotAt(const fui::DeviceContext& device, int x, int y);

constexpr int16_t encodeCard(const int slot, const scoundrel::Mode mode) {
  return static_cast<int16_t>(slot * 4 + static_cast<int>(mode));
}
inline int decodeSlot(const int16_t value) { return value / 4; }
inline scoundrel::Mode decodeMode(const int16_t value) { return static_cast<scoundrel::Mode>(value % 4); }

// The message line under the table, from the core's note. Upper case.
void noteText(const scoundrel::Game& game, char* out, int size);

void buildMenu(toybox::Screen& screen, const MenuModel& model);
void buildHowTo(toybox::Screen& screen, const HowToModel& model);
void buildBoard(toybox::Screen& screen, const BoardModel& model);
void buildResult(toybox::Screen& screen, const ResultModel& model);

int howToPages();

}  // namespace scoundrelui
