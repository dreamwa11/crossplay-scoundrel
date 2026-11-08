#include "ScoundrelActivity.h"

#include <HalStorage.h>
#include <Logging.h>
#include <Memory.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../Shelf.h"
#include "../ui/Toybox.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxSeed.h"
#include "../ui/ToyboxTheme.h"
#include "ScoundrelFonts.h"

namespace sc = scoundrel;
namespace ui = scoundrelui;

namespace {
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
constexpr char kSavePath[] = "/.crosspoint/scoundrel.sav";
#endif

// The stats line plus a full game: 44 cards and 26 kills at four bytes each
// fit with room to spare.
constexpr int kSaveBytes = 640;

namespace sf = scoundrelfonts;

// The phone's faces in the component slots too, so buttons and list rows match
// the text the screens draw: Space Grotesk, with Pirata One as the title.
toybox::Faces boardFaces() { return toybox::Faces{sf::kSmallId, sf::kLargeId, sf::kValueId}; }

int faceId(const ui::Face face) {
  switch (face) {
    case ui::Face::Small:
      return sf::kSmallId;
    case ui::Face::Body:
      return sf::kBodyId;
    case ui::Face::Large:
      return sf::kLargeId;
    case ui::Face::Value:
      return sf::kValueId;
    case ui::Face::ValueBig:
      return sf::kValueBigId;
    case ui::Face::Logo:
    default:
      return sf::kLogoId;
  }
}

// Lets the screens choose a face per string without knowing the renderer.
struct TargetFonts final : ui::FontBinder {
  freeink::ui::GfxRendererTarget& target;
  explicit TargetFonts(freeink::ui::GfxRendererTarget& t) : target(t) {}
  void bind(const freeink::ui::FontId slot, const ui::Face face) override { target.setFont(slot, faceId(face)); }
};
}  // namespace

std::unique_ptr<Activity> ScoundrelActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<ScoundrelActivity>(renderer, mappedInput);
}

void ScoundrelActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);
  sf::ensure(renderer);
  load();
  // Straight back into a dungeon in progress: the shelf already took two taps.
  screen = hasGame ? ui::Screen::Board : ui::Screen::Menu;
  requestUpdate();
}

void ScoundrelActivity::onExit() {
  save();
  Activity::onExit();
}

// File, version 1:
//   line 1: S1 maxHp played survived hasBest best
//   line 2: the game in progress, if any (ScoundrelCore's own format)
void ScoundrelActivity::load() {
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  hasGame = false;
  if (!Storage.exists(kSavePath)) return;
  char buffer[kSaveBytes] = {};
  if (Storage.readFileToBuffer(kSavePath, buffer, sizeof(buffer) - 1) == 0) return;
  int v[5] = {};
  if (std::strncmp(buffer, "S1 ", 3) != 0) return;
  char* cursor = buffer + 3;
  for (int& x : v) {
    char* next = nullptr;
    const long value = std::strtol(cursor, &next, 10);
    if (next == cursor) return;
    x = static_cast<int>(value);
    cursor = next;
  }
  if (v[0] >= sc::kMinHp && v[0] <= sc::kMaxHp) maxHp = v[0];
  played = v[1] < 0 ? 0 : v[1];
  survived = v[2] < 0 ? 0 : v[2];
  hasBest = v[3] != 0;
  best = v[4];
  char* line = std::strchr(cursor, '\n');
  if (!line) return;
  sc::Game g;
  if (sc::parse(line + 1, g) && !g.over) {
    game = g;
    hasGame = true;
  }
#endif
}

void ScoundrelActivity::save() {
#if defined(ARDUINO_ARCH_ESP32) || defined(SIMULATOR)
  char buffer[kSaveBytes];
  int used = std::snprintf(buffer, sizeof(buffer), "S1 %d %d %d %d %d\n", maxHp, played, survived, hasBest ? 1 : 0,
                           best);
  if (used <= 0 || used >= static_cast<int>(sizeof(buffer))) return;
  if (hasGame && !game.over) {
    const int n = sc::serialize(game, buffer + used, static_cast<int>(sizeof(buffer)) - used - 2);
    if (n < 0) {
      LOG_ERR("SCND", "Game did not fit %d bytes", kSaveBytes);
    } else {
      used += n;
      buffer[used++] = '\n';
      buffer[used] = '\0';
    }
  }
  Storage.writeFile(kSavePath, String(buffer));
#endif
}

void ScoundrelActivity::goTo(const ui::Screen next) {
  screen = next;
  requestUpdate();
}

void ScoundrelActivity::newGame() {
  sc::start(game, toybox::seed(), maxHp);
  hasGame = true;
  resultRecorded = false;
  newBest = false;
  save();
  goTo(ui::Screen::Board);
}

void ScoundrelActivity::recordResult() {
  if (resultRecorded) return;
  resultRecorded = true;
  ++played;
  if (game.won) ++survived;
  newBest = !hasBest || game.score > best;
  if (newBest) {
    best = game.score;
    hasBest = true;
  }
  hasGame = false;
  save();
}

void ScoundrelActivity::play(const int slot, const sc::Mode mode) {
  if (!sc::resolve(game, slot, mode)) return;
  if (game.over) {
    recordResult();
  } else {
    save();
  }
  requestUpdate();
}

uint32_t ScoundrelActivity::surfaceMeaning() const {
  const uint32_t withScreen = paintclock::mixMeaning(paintclock::kMeaningSeed, static_cast<uint32_t>(screen));
  // Which slots hold a card decides what a tap on the table does. The cards
  // themselves change only by the player's own last tap, so their identity is
  // left out: folding it in would gate every next move behind a full refresh.
  uint32_t occupied = 0;
  for (int slot = 0; slot < sc::kRoomSize; ++slot) occupied |= (game.room[slot] != sc::kNoCard ? 1u : 0u) << slot;
  const uint32_t withRoom = paintclock::mixMeaning(withScreen, occupied);
  return paintclock::mixMeaning(withRoom, game.over ? 1u : 0u);
}

void ScoundrelActivity::loop() {
  namespace fui = freeink::ui;

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    switch (screen) {
      case ui::Screen::Menu:
        save();
        shelf::leave(renderer, mappedInput);
        return;
      case ui::Screen::Confirm:
        goTo(ui::Screen::Board);
        return;
      case ui::Screen::Result:
      case ui::Screen::HowTo:
      case ui::Screen::Board:
        save();
        goTo(ui::Screen::Menu);
        return;
    }
  }

  fui::InputSnapshot input;
  int tapX = 0;
  int tapY = 0;
  if (!mappedInput.wasScreenTapped(tapX, tapY)) return;
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(tapX);
  input.touchY = static_cast<int16_t>(tapY);
  if (!interactionsReady) return;

  const fui::ActionEvent event = interactions.route(input);
  switch (event.action) {
    case ui::ActionMenuRow:
      switch (static_cast<ui::MenuRow>(event.value)) {
        case ui::MenuRow::Continue:
          if (hasGame) goTo(ui::Screen::Board);
          return;
        case ui::MenuRow::NewGame:
          newGame();
          return;
        case ui::MenuRow::HowTo:
          howToPage = 0;
          goTo(ui::Screen::HowTo);
          return;
        case ui::MenuRow::Health:
          // 10, 15, 20, 25, 30 and round again. Applies from the next dungeon.
          maxHp = maxHp + 5 > sc::kMaxHp ? sc::kMinHp : maxHp + 5;
          save();
          requestUpdate();
          return;
        case ui::MenuRow::Count:
          return;
      }
      return;

    case ui::ActionHowToNext:
      if (howToPage + 1 < ui::howToPages()) {
        ++howToPage;
        requestUpdate();
      } else {
        goTo(ui::Screen::Menu);
      }
      return;

    case ui::ActionCard:
      if (screen == ui::Screen::Board) play(ui::decodeSlot(event.value), ui::decodeMode(event.value));
      return;

    case ui::ActionFlee:
      if (screen != ui::Screen::Board) return;
      if (sc::flee(game)) {
        save();
      }
      // A refused flee still says why.
      requestUpdate();
      return;

    case ui::ActionSeeResult:
      goTo(ui::Screen::Result);
      return;

    case ui::ActionAgain:
      newGame();
      return;

    case ui::ActionDone:
      goTo(ui::Screen::Menu);
      return;

    case ui::ActionMenu:
      save();
      goTo(ui::Screen::Menu);
      return;

    case ui::ActionNewGame:
      // Like the phone: a dungeon you have started asks before it is thrown away.
      if (hasGame && game.started && !game.over) {
        goTo(ui::Screen::Confirm);
      } else {
        newGame();
      }
      return;

    case ui::ActionAbandon:
      newGame();
      return;

    case ui::ActionKeepPlaying:
      goTo(ui::Screen::Board);
      return;

    default:
      break;
  }

  // Nothing routed: a tap on a card body plays it the default way (the weapon
  // when it can hit, otherwise bare hands), like the web version.
  if (screen == ui::Screen::Board && !game.over) {
    const fui::DeviceContext device = toybox::makeTarget(renderer, boardFaces()).deviceContext();
    const int slot = ui::slotAt(device, tapX, tapY);
    if (slot < 0 || game.room[slot] == sc::kNoCard) return;
    if (!surfaceRevealed()) return;
    play(slot, sc::Mode::Auto);
  }
}

void ScoundrelActivity::render(RenderLock&&) {
  namespace fui = freeink::ui;

  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer, boardFaces());
  TargetFonts fonts(target);
  ui::useFonts(&fonts);
  const fui::DeviceContext device = target.deviceContext();
  const fui::InputSnapshot noInput{};
  interactionsReady = false;
  toybox::Frame frame(target, device, noInput, interactions);
  toybox::Screen surface(frame);

  switch (screen) {
    case ui::Screen::Menu: {
      ui::MenuModel model;
      model.canContinue = hasGame;
      model.maxHp = maxHp;
      model.played = played;
      model.survived = survived;
      model.hasBest = hasBest;
      model.best = best;
      ui::buildMenu(surface, model);
      break;
    }
    case ui::Screen::HowTo: {
      ui::HowToModel model;
      model.page = howToPage;
      ui::buildHowTo(surface, model);
      break;
    }
    case ui::Screen::Board: {
      ui::BoardModel model;
      model.game = game;
      ui::buildBoard(surface, model);
      break;
    }
    case ui::Screen::Confirm:
      ui::buildConfirm(surface);
      break;
    case ui::Screen::Result: {
      ui::ResultModel model;
      model.game = game;
      model.newBest = newBest;
      model.best = best;
      ui::buildResult(surface, model);
      break;
    }
  }

  ui::useFonts(nullptr);
  interactionsReady = true;
  toybox::reportOverflow(interactions, "Scoundrel");

  const auto labels = mappedInput.mapLabels("Back", "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
