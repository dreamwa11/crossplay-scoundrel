#pragma once

// Scoundrel on screen. Freestanding builders over plain models: FreeInkUI and
// Toybox tokens, nothing else, so host-tests/ui can build every screen against a
// fake target.
//
// The table follows the phone version's arrangement: a top bar (cards and
// monsters left, the room, a menu button), the room as one row of four cards
// with their choices underneath, then the desk -- deck, hero, weapon -- and
// NEW DUNGEON / FLEE ROOM at the bottom. Every card carries its kind's mark
// top-left and its value top-right, as on the phone.

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
  // The board's menu button and its NEW DUNGEON button, and the confirm sheet.
  ActionMenu = 8,
  ActionNewGame = 9,
  ActionAbandon = 10,
  ActionKeepPlaying = 11,
};

enum class Screen : uint8_t { Menu = 0, HowTo, Board, Result, Confirm };

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

// Text faces. The screens draw their own text in the phone's three families;
// the activity binds the one asked for into a spare font slot just before
// each string is drawn.
enum class Face : uint8_t { Small = 0, Body, Large, Value, ValueBig, Logo, Title };

struct FontBinder {
  virtual ~FontBinder() = default;
  // Bind `face` into font slot `slot` of the target being drawn on.
  virtual void bind(fui::FontId slot, Face face) = 0;
};
void useFonts(FontBinder* binder);

// The rect of a room slot, and its inverse. Card bodies are hit-tested from the
// same geometry that drew them; the chips under them are ordinary buttons.
fui::Rect slotRect(const fui::DeviceContext& device, int slot);
int slotAt(const fui::DeviceContext& device, int x, int y);

constexpr int16_t encodeCard(const int slot, const scoundrel::Mode mode) {
  return static_cast<int16_t>(slot * 4 + static_cast<int>(mode));
}
inline int decodeSlot(const int16_t value) { return value / 4; }
inline scoundrel::Mode decodeMode(const int16_t value) { return static_cast<scoundrel::Mode>(value % 4); }

// The message strip, from the core's note, in the phone version's words.
// Empty for a new dungeon: the strip then shows the logo instead.
void noteText(const scoundrel::Game& game, char* out, int size);

void buildMenu(toybox::Screen& screen, const MenuModel& model);
void buildHowTo(toybox::Screen& screen, const HowToModel& model);
void buildBoard(toybox::Screen& screen, const BoardModel& model);
void buildResult(toybox::Screen& screen, const ResultModel& model);
void buildConfirm(toybox::Screen& screen);

int howToPages();

}  // namespace scoundrelui
