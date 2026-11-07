#pragma once

// Scoundrel on the device. The thin layer: renderer, storage, input, shelf.
// Rules live in ScoundrelCore, pixels in ScoundrelScreens.

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "ScoundrelCore.h"
#include "ScoundrelScreens.h"

class ScoundrelActivity final : public Activity {
 public:
  ScoundrelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Scoundrel", renderer, mappedInput) {}
  ~ScoundrelActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void goTo(scoundrelui::Screen next);
  void newGame();
  void play(int slot, scoundrel::Mode mode);
  void recordResult();
  void load();
  void save();

  scoundrelui::Screen screen = scoundrelui::Screen::Menu;
  scoundrel::Game game{};
  bool hasGame = false;  // a dungeon in progress, resumable from the menu
  int maxHp = scoundrel::kDefaultHp;
  int played = 0;
  int survived = 0;
  bool hasBest = false;
  int best = 0;
  bool newBest = false;
  bool resultRecorded = false;
  int howToPage = 0;

  toybox::Interactions interactions;
  bool interactionsReady = false;

  // The room is hit-tested against geometry, so the table digest cannot see a
  // tap on a card body. See Activity::surfaceMeaning().
  uint32_t surfaceMeaning() const override;
};
