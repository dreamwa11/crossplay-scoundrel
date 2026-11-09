#include "ScoundrelScreens.h"

#include <FreeInkUIIcon.h>

#include <cctype>
#include <cstdio>
#include <cstring>

#include "../cards/CardArt.h"
#include "../ui/ToyboxFormat.h"
#include "ScoundrelArt.h"
#include "ScoundrelScene.h"
#include "fonts/ScoundrelCuts.h"

namespace scoundrelui {

namespace {

namespace s = scoundrel;
namespace sf = scoundrelfonts;

const fui::Paint kBlack = fui::Paint::solid(fui::Color::Black);
const fui::Paint kWhite = fui::Paint::solid(fui::Color::White);
const fui::Paint kGrey = fui::Paint::dither(fui::Color::DarkGray);
constexpr int16_t kRadius = 8;

// The slot the screens rebind for their own text. FONT_LABEL is the fourth
// slot GfxRendererTarget offers, and nothing here uses it for chrome.
constexpr fui::FontId kTextSlot = 3;
FontBinder* gFonts = nullptr;

const toybox::CutMetrics& cutOf(const Face face) {
  switch (face) {
    case Face::Small:
      return sf::kSmallCut;
    case Face::Body:
      return sf::kBodyCut;
    case Face::Large:
      return sf::kLargeCut;
    case Face::Value:
      return sf::kValueCut;
    case Face::ValueBig:
      return sf::kValueBigCut;
    case Face::Title:
      return sf::kTitleCut;
    case Face::Logo:
    default:
      return sf::kLogoCut;
  }
}

fui::TextStyle face(const Face f, const fui::TextAlign align = fui::TextAlign::Center, const bool paper = false) {
  if (gFonts) gFonts->bind(kTextSlot, f);
  fui::TextStyle t;
  t.font = kTextSlot;
  t.align = align;
  if (paper) t.color = fui::Color::White;
  return t;
}

// Draws `text` with its capitals centred in `box` (or aligned left/right).
void say(toybox::Screen& screen, const fui::Rect& box, const char* text, const Face f,
         const fui::TextAlign align = fui::TextAlign::Center, const bool paper = false) {
  const fui::TextStyle t = face(f, align, paper);
  screen.target().text(toybox::inkCentred(box, cutOf(f)), text, t);
}

int16_t widthOf(toybox::Screen& screen, const char* text, const Face f) {
  const fui::TextStyle t = face(f);
  return screen.target().measureText(t.font, text, t).width;
}

void upper(const char* in, char* out, const size_t size) {
  size_t i = 0;
  for (; in[i] && i + 1 < size; ++i) out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(in[i])));
  out[i] = '\0';
}

void lower(const char* in, char* out, const size_t size) {
  size_t i = 0;
  for (; in[i] && i + 1 < size; ++i) out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(in[i])));
  out[i] = '\0';
}

// ---- geometry -------------------------------------------------------------
//
// The phone version's arrangement, top to bottom, on a 480x800 panel:
//
//   top bar      cards left / monsters left, the room, the menu button
//   the scene    a doorway in a brick wall
//   the room     four cards in a row, their choices underneath
//   strip        the logo, or what just happened
//   the desk     DECK, HEALTH, WEAPON
//   buttons      NEW DUNGEON (until the first move), FLEE ROOM
constexpr int16_t kTopBarH = 58;
constexpr int16_t kSceneBottom = 238;
constexpr int16_t kRoomTop = 246;
constexpr int16_t kCardW = 110;
constexpr int16_t kCardH = 150;
constexpr int16_t kRoomGap = 8;
constexpr int16_t kChipTop = kRoomTop + kCardH + 6;
constexpr int16_t kChipH = 40;
constexpr int16_t kStripTop = kChipTop + kChipH + 8;
constexpr int16_t kStripH = 48;
constexpr int16_t kDeskTop = kStripTop + kStripH + 14;
constexpr int16_t kDeskW = 104;
constexpr int16_t kDeskH = 150;
constexpr int16_t kButtonsTop = 724;

int16_t roomLeft(const fui::DeviceContext& device) {
  return static_cast<int16_t>((device.width - (kCardW * 4 + kRoomGap * 3)) / 2);
}

fui::Rect centred(const int16_t cx, const int16_t y, const int16_t w, const int16_t h) {
  return fui::makeRect(static_cast<int16_t>(cx - w / 2), y, w, h);
}

const freeink::Icon& artFor(const uint8_t card, const int size) {
  const int v = s::valueOf(card);
  const bool b = size >= 96;
  const bool m = size >= 64 && !b;
#define ART(name) (b ? icon_##name##_96 : m ? icon_##name##_64 : icon_##name##_48)
  switch (s::kindOf(card)) {
    case s::Kind::Monster:
      if (v == 11) return ART(jack);
      if (v == 12) return ART(queen);
      if (v == 13) return ART(king);
      if (v == 14) return ART(ace);
      if (v == 2) return ART(goblin);
      if (v <= 4) return ART(wolf);
      if (v <= 6) return ART(skeleton);
      if (v <= 8) return ART(orc);
      return ART(beast);
    case s::Kind::Weapon:
      if (v <= 3) return ART(dagger);
      if (v <= 5) return ART(shortsword);
      if (v <= 8) return ART(longsword);
      return ART(greatsword);
    case s::Kind::Potion:
    default:
      if (v <= 3) return ART(potionsmall);
      if (v <= 8) return ART(potion);
      return ART(potionlarge);
  }
#undef ART
}

const freeink::Icon& markFor(const uint8_t card) {
  switch (s::kindOf(card)) {
    case s::Kind::Monster:
      return icon_skull_24;
    case s::Kind::Weapon:
      return icon_sword_24;
    default:
      return icon_heart_24;
  }
}

void icon(toybox::Screen& screen, const fui::Rect& box, const freeink::Icon& art, const bool paper = false) {
  screen.target().bitmap(box, fui::bitmapFromIcon(art), fui::BitmapMode::Center, paper ? kWhite : kBlack);
}

// ---- the card face --------------------------------------------------------
//
// Kind mark top-left, value top-right in Scoundrel Gothic, the picture, the name in
// small capitals along the bottom: the phone's card, in black and white.
void drawFace(toybox::Screen& screen, const fui::Rect& rect, const uint8_t card, const int art) {
  auto& target = screen.target();
  target.fill(rect, kWhite, kRadius);
  target.stroke(rect, kBlack, 2, kRadius);
  icon(screen, fui::makeRect(static_cast<int16_t>(rect.x + 7), static_cast<int16_t>(rect.y + 8), 24, 24),
       markFor(card));
  say(screen, fui::makeRect(static_cast<int16_t>(rect.right() - 8 - 64), static_cast<int16_t>(rect.y + 4), 64, 34),
      s::rankLabel(card), Face::Value, fui::TextAlign::Right);

  const int16_t nameH = sf::kSmallCut.lineHeight;
  const int16_t nameTop = static_cast<int16_t>(rect.bottom() - nameH - 4);
  const int16_t room = static_cast<int16_t>(nameTop - rect.y - 38);
  const int16_t artTop = static_cast<int16_t>(rect.y + 38 + (room > art ? (room - art) / 2 : 0));
  icon(screen, centred(static_cast<int16_t>(rect.x + rect.width / 2), artTop, static_cast<int16_t>(art),
                       static_cast<int16_t>(art)),
       artFor(card, art));
  char name[24];
  upper(s::cardName(card), name, sizeof(name));
  say(screen, fui::makeRect(static_cast<int16_t>(rect.x + 2), nameTop, static_cast<int16_t>(rect.width - 4), nameH),
      name, Face::Small);
}

// One choice under a room card: outlined, or solid black when it is the
// better of two; a choice that would kill you carries a skull instead of its
// tool. Registered bare and drawn here, so a 24px mark and a number fit 53px.
void chip(toybox::Screen& screen, const fui::Rect& box, const char* label, const freeink::Icon* art, const bool loud,
          const int16_t value) {
  fui::ButtonProps b;
  b.label = "";
  b.action = ActionCard;
  b.value = value;
  b.styles = toybox::rowStyles();
  b.state = loud ? fui::StateSelected : fui::StateNormal;
  b.radius = 8;
  b.minTouchSize = 40;
  screen.button(b, box);

  const int16_t tw = widthOf(screen, label, Face::Body);
  const int16_t iw = art ? 24 : 0;
  const int16_t gap = art ? 4 : 0;
  const int16_t x = static_cast<int16_t>(box.x + (box.width - iw - gap - tw) / 2);
  if (art) icon(screen, fui::makeRect(x, static_cast<int16_t>(box.y + (box.height - 24) / 2), 24, 24), *art, loud);
  say(screen, fui::makeRect(static_cast<int16_t>(x + iw + gap), box.y, static_cast<int16_t>(tw + 6), box.height), label,
      Face::Body, fui::TextAlign::Left, loud);
}

// The weapon choice when there is no weapon, or it is too dull: shown, not
// offered, like the phone's greyed-out sword.
void spentChip(toybox::Screen& screen, const fui::Rect& box) {
  auto& target = screen.target();
  target.stroke(box, kGrey, 2, 8);
  const int16_t x = static_cast<int16_t>(box.x + (box.width - 24 - 4 - 10) / 2);
  target.bitmap(fui::makeRect(x, static_cast<int16_t>(box.y + (box.height - 24) / 2), 24, 24),
                fui::bitmapFromIcon(icon_sword_24), fui::BitmapMode::Center, kGrey);
  say(screen, fui::makeRect(static_cast<int16_t>(x + 28), box.y, 14, box.height), "\xE2\x80\x93", Face::Body,
      fui::TextAlign::Left);
}

void drawRoomCard(toybox::Screen& screen, const BoardModel& model, const int slot) {
  const fui::Rect rect = slotRect(screen.device(), slot);
  const uint8_t card = model.game.room[slot];
  if (card == s::kNoCard) {
    cardart::drawCardSlot(screen, rect);
    return;
  }
  drawFace(screen, rect, card, 64);
  if (model.game.over) return;

  const fui::Rect row = fui::makeRect(rect.x, kChipTop, rect.width, kChipH);
  const s::Game& g = model.game;
  char a[12];
  char b[12];
  switch (s::kindOf(card)) {
    case s::Kind::Monster: {
      const s::Preview p = s::preview(g, card);
      const int16_t half = static_cast<int16_t>((row.width - 4) / 2);
      const fui::Rect left = fui::makeRect(row.x, row.y, half, row.height);
      const fui::Rect right = fui::makeRect(static_cast<int16_t>(row.right() - half), row.y, half, row.height);
      std::snprintf(a, sizeof(a), "%d", p.fist);
      if (p.weapon < 0) {
        chip(screen, left, a, p.fist >= g.hp ? &icon_skull_24 : &icon_hand_24, false, encodeCard(slot, s::Mode::Fist));
        spentChip(screen, right);
        break;
      }
      std::snprintf(b, sizeof(b), "%d", p.weapon);
      // The weapon is the loud one unless bare hands cost no more.
      const bool fistBetter = p.fist <= p.weapon;
      chip(screen, left, a, p.fist >= g.hp ? &icon_skull_24 : &icon_hand_24, fistBetter,
           encodeCard(slot, s::Mode::Fist));
      chip(screen, right, b, p.weapon >= g.hp ? &icon_skull_24 : &icon_sword_24, !fistBetter,
           encodeCard(slot, s::Mode::Weapon));
      break;
    }
    case s::Kind::Weapon:
      chip(screen, row, g.weapon == s::kNoCard ? "equip" : "swap", &icon_sword_24, false,
           encodeCard(slot, s::Mode::Auto));
      break;
    case s::Kind::Potion:
    default:
      if (g.potionUsed) {
        // Taking it does nothing but clear it away: the second potion of a room.
        chip(screen, row, "waste", nullptr, false, encodeCard(slot, s::Mode::Auto));
      } else {
        int heal = g.maxHp - g.hp;
        if (heal > s::valueOf(card)) heal = s::valueOf(card);
        std::snprintf(a, sizeof(a), "+%d", heal);
        chip(screen, row, a, &icon_heart_24, false, encodeCard(slot, s::Mode::Auto));
      }
      break;
  }
}

// ---- top bar, scene, strip --------------------------------------------------

void topBar(toybox::Screen& screen, const s::Game& g) {
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  char line[16];
  int16_t x = 12;
  const int16_t midY = kTopBarH / 2;
  auto counter = [&](const freeink::Icon& art, const int n) {
    icon(screen, fui::makeRect(x, static_cast<int16_t>(midY - 12), 24, 24), art);
    std::snprintf(line, sizeof(line), "%d", n);
    const int16_t tw = widthOf(screen, line, Face::Large);
    say(screen, fui::makeRect(static_cast<int16_t>(x + 28), 0, static_cast<int16_t>(tw + 4), kTopBarH), line, Face::Large,
        fui::TextAlign::Left);
    x = static_cast<int16_t>(x + 28 + tw + 14);
  };
  counter(icon_deck_24, g.deckLen + s::roomCount(g));
  counter(icon_skull_24, s::monstersLeft(g));

  // The room's name, centred on the panel like the phone's.
  std::snprintf(line, sizeof(line), "Room %d", g.roomNo);
  say(screen, fui::makeRect(0, 0, w, kTopBarH), line, Face::Value);

  fui::ButtonProps menu;
  menu.icon = fui::bitmapFromIcon(icon_menu_24);
  menu.action = ActionMenu;
  menu.styles = toybox::rowStyles();
  menu.radius = 8;
  screen.button(menu, fui::makeRect(static_cast<int16_t>(w - 12 - 46), static_cast<int16_t>(midY - 23), 46, 46));
  target.fill(fui::makeRect(0, kTopBarH, w, 2), kBlack);
}

// The doorway in a brick wall, as on the phone. It never changes, so it costs
// no ghosting.
void scene(toybox::Screen& screen, const int roomNo) {
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  const int16_t top = kTopBarH + 2;
  // A drawn stone room: walls of varied, cracked and broken bricks, an arched
  // doorway and two torches. Three walls take turns from room to room.
  const freeink::Icon& art = *kRoomScenes[(roomNo > 0 ? roomNo - 1 : 0) % kRoomSceneCount];
  const fui::Rect band = fui::makeRect(0, top, w, kSceneBottom - top);
  target.fill(band, kWhite);
  target.bitmap(band, fui::bitmapFromIcon(art), fui::BitmapMode::Center, kBlack);
  target.fill(fui::makeRect(0, kSceneBottom, w, kStripTop - kSceneBottom), kWhite);
  target.fill(fui::makeRect(0, kSceneBottom, w, 2), kBlack);
}

// The strip between the room and the desk: the logo on a fresh dungeon, then
// whatever just happened. One black band, as on the phone.
void strip(toybox::Screen& screen, const s::Game& g) {
  const int16_t w = screen.device().width;
  const fui::Rect band = fui::makeRect(0, kStripTop, w, kStripH);
  screen.target().fill(band, kBlack);
  char note[64];
  noteText(g, note, sizeof(note));
  if (note[0] == '\0') {
    say(screen, band, "Scoundrel", Face::Logo, fui::TextAlign::Center, true);
  } else {
    say(screen, fui::makeRect(10, band.y, static_cast<int16_t>(w - 20), band.height), note, Face::Body,
        fui::TextAlign::Center, true);
  }
}

// ---- the desk ---------------------------------------------------------------

fui::Rect deskRect(const fui::DeviceContext& device, const int i) {
  const int16_t gap = static_cast<int16_t>((device.width - kDeskW * 3) / 4);
  return fui::makeRect(static_cast<int16_t>(gap + i * (kDeskW + gap)), kDeskTop, kDeskW, kDeskH);
}

void deskLabel(toybox::Screen& screen, const fui::Rect& slot, const char* label) {
  say(screen,
      fui::makeRect(static_cast<int16_t>(slot.x - 20), static_cast<int16_t>(slot.bottom() + 20),
                    static_cast<int16_t>(slot.width + 40), sf::kSmallCut.lineHeight),
      label, Face::Small);
}

// A white pill hanging off the bottom of a desk card: the deck's count.
void badge(toybox::Screen& screen, const fui::Rect& slot, const char* text, const int16_t width) {
  const fui::Rect pill = centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.bottom() - 18),
                                 width, 36);
  screen.target().fill(pill, kWhite, 14);
  screen.target().stroke(pill, kBlack, 2, 14);
  say(screen, pill, text, Face::Value);
}

void drawDeck(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  auto& target = screen.target();
  if (g.deckLen == 0) {
    cardart::drawCardSlot(screen, slot);
    say(screen, slot, "Deck empty", Face::Small);
  } else {
    // A few backs offset behind the top one, like the phone's pile.
    const int backs = g.deckLen > 3 ? 3 : g.deckLen - 1;
    for (int i = backs; i >= 0; --i) {
      const fui::Rect card = fui::makeRect(static_cast<int16_t>(slot.x + 2 * i), static_cast<int16_t>(slot.y - 2 * i),
                                           slot.width, slot.height);
      target.fill(card, kWhite, kRadius);
      target.stroke(card, kBlack, 2, kRadius);
      if (i != 0) continue;
      const fui::Rect inner = fui::makeRect(static_cast<int16_t>(card.x + 7), static_cast<int16_t>(card.y + 7),
                                            static_cast<int16_t>(card.width - 14),
                                            static_cast<int16_t>(card.height - 14));
      target.fill(inner, kGrey, kRadius / 2);
      target.stroke(inner, kBlack, 2, kRadius / 2);
      const fui::Rect halo = centred(static_cast<int16_t>(card.x + card.width / 2),
                                     static_cast<int16_t>(card.y + card.height / 2 - 34), 52, 52);
      target.fill(halo, kWhite, 26);
      target.stroke(halo, kBlack, 2, 26);
      icon(screen, centred(static_cast<int16_t>(halo.x + 26), static_cast<int16_t>(halo.y + 10), 32, 32),
           icon_skull_32);
    }
    char line[8];
    std::snprintf(line, sizeof(line), "%d", g.deckLen);
    badge(screen, slot, line, 58);
  }
  deskLabel(screen, slot, "DECK");
}

// The hero: no frame, so it does not read as one more card. The portrait,
// then "17/20" in Scoundrel Gothic and a thin health bar.
void drawHero(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  auto& target = screen.target();
  icon(screen, centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.y - 4), 96, 96),
       icon_hero_96);
  const int hp = g.hp < 0 ? 0 : g.hp;
  char big[8];
  char small[8];
  std::snprintf(big, sizeof(big), "%d", hp);
  std::snprintf(small, sizeof(small), "/%d", g.maxHp);
  const int16_t bw = widthOf(screen, big, Face::ValueBig);
  const int16_t sw = widthOf(screen, small, Face::Value);
  const int16_t x = static_cast<int16_t>(slot.x + (slot.width - bw - sw - 2) / 2);
  // Both cuts sit on one baseline: the big one's box is placed, the small one
  // drops by the difference in their cap heights.
  const int16_t numberTop = static_cast<int16_t>(slot.y + 96);
  const int16_t numberH = 44;
  say(screen, fui::makeRect(x, numberTop, static_cast<int16_t>(bw + 4), numberH), big, Face::ValueBig,
      fui::TextAlign::Left);
  const int16_t drop = static_cast<int16_t>((sf::kValueBigCut.inkHeight - sf::kValueCut.inkHeight) / 2);
  say(screen, fui::makeRect(static_cast<int16_t>(x + bw + 2), static_cast<int16_t>(numberTop + drop),
                            static_cast<int16_t>(sw + 4), numberH),
      small, Face::Value, fui::TextAlign::Left);
  const fui::Rect bar = fui::makeRect(static_cast<int16_t>(slot.x + 6), static_cast<int16_t>(slot.bottom() - 6),
                                      static_cast<int16_t>(slot.width - 12), 8);
  target.stroke(bar, kBlack, 1, 3);
  const int16_t filled = static_cast<int16_t>(bar.width * hp / (g.maxHp > 0 ? g.maxHp : 1));
  if (filled > 0) target.fill(fui::makeRect(bar.x, bar.y, filled, bar.height), kBlack, 3);
  deskLabel(screen, slot, "HEALTH");
}

// The weapon, with its kills laid over it as on the phone. Each kill covers
// the one before but leaves the weapon's top strip, and its value, in view.
void drawWeaponSlot(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  if (g.weapon == s::kNoCard) {
    cardart::drawCardSlot(screen, slot);
    icon(screen, centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.y + 46), 32, 32),
         icon_hand_32);
    say(screen, fui::makeRect(slot.x, static_cast<int16_t>(slot.y + 86), slot.width, sf::kSmallCut.lineHeight),
        "NO WEAPON", Face::Small);
    deskLabel(screen, slot, "WEAPON");
    return;
  }
  drawFace(screen, slot, g.weapon, 64);
  const int first = g.slainLen > 4 ? g.slainLen - 4 : 0;
  for (int i = first; i < g.slainLen; ++i) {
    const int k = i - first;
    const int16_t top = static_cast<int16_t>(slot.y + 40 + k * 10);
    const fui::Rect kill = fui::makeRect(static_cast<int16_t>(slot.x + 6 + k * 4), top,
                                         static_cast<int16_t>(slot.width - 6), static_cast<int16_t>(slot.bottom() + 8 - top));
    drawFace(screen, kill, g.slain[i], 48);
  }
  char line[24];
  if (g.weaponLast >= 0) {
    std::snprintf(line, sizeof(line), "HITS \xE2\x89\xA4 %s", s::rankLabel(s::makeCard(s::Kind::Monster, g.weaponLast)));
  } else {
    std::snprintf(line, sizeof(line), "WEAPON");
  }
  deskLabel(screen, slot, line);
}

// A title the way the phone's sheets set theirs: Astloch, centred, a rule.
void title(toybox::Screen& screen, const char* text) {
  const int16_t w = screen.device().width;
  say(screen, fui::makeRect(0, 14, w, 64), text, Face::Logo);
  screen.target().fill(fui::makeRect(24, 88, static_cast<int16_t>(w - 48), 2), kBlack);
  screen.setContentMarginAbsolute(fui::Insets{104, toybox::kMargin, toybox::kMargin, toybox::kMargin});
}

fui::ButtonProps pill(const char* label, const fui::ActionId action, const bool solid = true) {
  fui::ButtonProps b;
  b.label = label;
  b.action = action;
  b.radius = 10;
  if (!solid) b.styles = toybox::rowStyles();
  return b;
}

// Wrapped prose in the large face, centred, up to `lines` lines.
void prose(toybox::Screen& screen, const fui::Rect& box, const char* text, const uint8_t lines) {
  fui::TextStyle t = face(Face::Large);
  t.maxLines = lines;
  screen.target().text(box, text, t);
}

}  // namespace

void useFonts(FontBinder* binder) { gFonts = binder; }

fui::Rect slotRect(const fui::DeviceContext& device, const int slot) {
  return fui::makeRect(static_cast<int16_t>(roomLeft(device) + slot * (kCardW + kRoomGap)), kRoomTop, kCardW, kCardH);
}

int slotAt(const fui::DeviceContext& device, const int x, const int y) {
  for (int slot = 0; slot < s::kRoomSize; ++slot) {
    const fui::Rect r = slotRect(device, slot);
    if (x >= r.x && x < r.right() && y >= r.y && y < r.bottom()) return slot;
  }
  return -1;
}

void noteText(const s::Game& g, char* out, const int size) {
  const char* name = g.noteCard == s::kNoCard ? "" : s::cardName(g.noteCard);
  const size_t n = static_cast<size_t>(size);
  char lowered[24];
  switch (g.note) {
    case s::Note::SlewWithWeapon:
      if (g.noteAmount > 0) {
        std::snprintf(out, n, "Slew the %s. Took %d damage.", name, g.noteAmount);
      } else {
        std::snprintf(out, n, "Slew the %s. No damage taken.", name);
      }
      return;
    case s::Note::TooDull:
      std::snprintf(out, n, "Too dull for the %s. Bare hands: took %d.", name, g.noteAmount);
      return;
    case s::Note::BareHands:
      std::snprintf(out, n, "Fought the %s bare-handed. Took %d.", name, g.noteAmount);
      return;
    case s::Note::Equipped:
      lower(name, lowered, sizeof(lowered));
      std::snprintf(out, n, "Equipped the %s (%d).", lowered, s::valueOf(g.noteCard));
      return;
    case s::Note::Drank:
      if (g.noteAmount > 0) {
        std::snprintf(out, n, "Drank a potion: +%d HP.", g.noteAmount);
      } else {
        std::snprintf(out, n, "Drank a potion. Already at full health.");
      }
      return;
    case s::Note::Wasted:
      std::snprintf(out, n, "One potion per room. This one is wasted.");
      return;
    case s::Note::Fled:
      std::snprintf(out, n, "You fled to the next room.");
      return;
    case s::Note::CannotFlee:
      std::snprintf(out, n, "You cannot flee two rooms in a row!");
      return;
    case s::Note::NewGame:
    case s::Note::None:
    default:
      if (size > 0) out[0] = '\0';
      return;
  }
}

// ---- menu -----------------------------------------------------------------

void buildMenu(toybox::Screen& screen, const MenuModel& model) {
  toybox::absoluteChrome(screen);
  title(screen, "Scoundrel");

  char record[64];
  if (model.hasBest) {
    std::snprintf(record, sizeof(record), "%d played    %d survived    best %d", model.played,
                  model.survived, model.best);
  } else {
    std::snprintf(record, sizeof(record), "%d played    %d survived", model.played, model.survived);
  }
  const fui::Rect line = screen.takeTop(26);
  say(screen, line, record, Face::Body);

  static char healthLabel[24];
  std::snprintf(healthLabel, sizeof(healthLabel), "%d HP", model.maxHp);
  fui::ListItem rows[static_cast<int>(MenuRow::Count)] = {};
  int16_t count = 0;
  if (model.canContinue) {
    rows[count].label = "Back to the dungeon";
    rows[count].actionValue = static_cast<int16_t>(MenuRow::Continue);
    ++count;
  }
  rows[count].label = "New dungeon";
  rows[count].actionValue = static_cast<int16_t>(MenuRow::NewGame);
  ++count;
  rows[count].label = "How to play";
  rows[count].actionValue = static_cast<int16_t>(MenuRow::HowTo);
  ++count;
  rows[count].label = "Maximum health";
  rows[count].value = healthLabel;
  rows[count].actionValue = static_cast<int16_t>(MenuRow::Health);
  ++count;

  fui::ListProps list;
  list.items = rows;
  list.count = static_cast<uint16_t>(count);
  list.selectedIndex = 0;
  list.action = ActionMenuRow;
  const int16_t listHeight =
      static_cast<int16_t>(count * toybox::kRowHeight + (count - 1) * toybox::kGutter / 2 + toybox::kGutter);
  const fui::Rect content = screen.contentRect();
  const int16_t creditH = 30;
  const int16_t listTop = static_cast<int16_t>(content.bottom() - listHeight - creditH);
  say(screen, fui::makeRect(content.x, static_cast<int16_t>(content.bottom() - creditH + 6), content.width, 24),
      "Original game by Zach Gage and Kurt Bieg, 2011", Face::Small);
  screen.takeBottom(creditH, 0);
  screen.list(list, listHeight, fui::LayoutAnchor::Bottom);

  // The ornament: one card of each kind, fanned.
  const int16_t areaTop = static_cast<int16_t>(line.bottom() + 6);
  const int16_t room = static_cast<int16_t>(listTop - areaTop);
  const int16_t w = 136;
  const int16_t h = 190;
  if (room >= h + 20) {
    const int16_t top = static_cast<int16_t>(areaTop + (room - h) / 2);
    const int16_t centre = static_cast<int16_t>(content.x + content.width / 2);
    const uint8_t fan[3] = {s::makeCard(s::Kind::Weapon, 7), s::makeCard(s::Kind::Monster, 14, 1),
                            s::makeCard(s::Kind::Potion, 9)};
    const int16_t offsets[3] = {-136, 0, 136};
    const int16_t drops[3] = {14, 0, 14};
    for (int i : {0, 2, 1}) {
      drawFace(screen,
               fui::makeRect(static_cast<int16_t>(centre - w / 2 + offsets[i]), static_cast<int16_t>(top + drops[i]), w,
                             h),
               fan[i], 96);
    }
  }
}

// ---- how to play ----------------------------------------------------------

namespace {

struct Lesson {
  const char* text;
  uint8_t cards[3];
};

const Lesson kLessons[] = {
    {"You are a scoundrel delving into a dungeon built from a deck of 44 cards. Clear every card while staying "
     "alive.",
     {0x40 | 0x0C, 0x10 | 0x05, 0x20 | 0x06}},
    {"Each room shows 4 cards. Once you have handled 3, the room refills from the deck. The card you left behind "
     "stays.",
     {0x00 | 0x02, 0x10 | 0x01, 0x20 | 0x03}},
    {"Monsters hit for their number. Fight bare-handed and take it all, or use your weapon and take only the "
     "difference.",
     {0x00 | 0x07, 0x10 | 0x03, s::kNoCard}},
    {"Weapons dull: once it has a kill, a weapon only hits monsters equal to or weaker than its last kill.",
     {0x00 | 0x09, 0x10 | 0x07, 0x40 | 0x0B}},
    {"Potions heal up to your maximum health. Only one per room counts; a second is wasted.",
     {0x20 | 0x01, 0x20 | 0x07, s::kNoCard}},
    {"Before touching a full room you may flee: its cards go to the bottom of the deck. Never two rooms in a row.",
     {s::kNoCard, s::kNoCard, s::kNoCard}},
};

}  // namespace

int howToPages() { return static_cast<int>(sizeof(kLessons) / sizeof(kLessons[0])); }

void buildHowTo(toybox::Screen& screen, const HowToModel& model) {
  const int pages = howToPages();
  const int page = model.page < 0 ? 0 : (model.page >= pages ? pages - 1 : model.page);
  toybox::absoluteChrome(screen);
  title(screen, "How to play");

  screen.button(pill(page + 1 < pages ? "Next" : "Into the dungeon", ActionHowToNext),
                screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  char progress[16];
  std::snprintf(progress, sizeof(progress), "%d of %d", page + 1, pages);
  say(screen, screen.takeBottom(28, 4), progress, Face::Small);

  const fui::Rect area = screen.body();
  prose(screen, fui::makeRect(area.x, area.y, area.width, 200), kLessons[page].text, 6);

  int n = 0;
  for (const uint8_t c : kLessons[page].cards) n += c != s::kNoCard;
  if (n == 0) return;
  const int16_t w = 132;
  const int16_t h = 190;
  const int16_t gap = 14;
  const int16_t total = static_cast<int16_t>(n * w + (n - 1) * gap);
  int16_t x = static_cast<int16_t>(area.x + (area.width - total) / 2);
  const int16_t y = static_cast<int16_t>(area.y + 230);
  for (const uint8_t c : kLessons[page].cards) {
    if (c == s::kNoCard) continue;
    drawFace(screen, fui::makeRect(x, y, w, h), c, 96);
    x = static_cast<int16_t>(x + w + gap);
  }
}

// ---- the table ------------------------------------------------------------

void buildBoard(toybox::Screen& screen, const BoardModel& model) {
  const s::Game& g = model.game;
  const fui::DeviceContext device = screen.device();
  toybox::absoluteChrome(screen);

  scene(screen, g.roomNo);
  topBar(screen, g);
  for (int slot = 0; slot < s::kRoomSize; ++slot) drawRoomCard(screen, model, slot);
  strip(screen, g);

  drawDeck(screen, deskRect(device, 0), g);
  drawHero(screen, deskRect(device, 1), g);
  drawWeaponSlot(screen, deskRect(device, 2), g);

  const int16_t side = 12;
  const fui::Rect buttons =
      fui::makeRect(side, kButtonsTop, static_cast<int16_t>(device.width - side * 2), toybox::kPillHeight);
  if (g.over) {
    screen.button(pill(g.won ? "You survived" : "You died", ActionSeeResult), buttons);
    return;
  }
  // As on the phone: NEW DUNGEON only until the first card is faced, then
  // FLEE ROOM has the bar to itself.
  fui::Rect fleeRect = buttons;
  if (!g.started) {
    const int16_t half = static_cast<int16_t>((buttons.width - side) / 2);
    screen.button(pill("New dungeon", ActionNewGame, false), fui::makeRect(buttons.x, buttons.y, half, buttons.height));
    fleeRect = fui::makeRect(static_cast<int16_t>(buttons.right() - half), buttons.y, half, buttons.height);
  }
  const char* fleeLabel = g.fledLast ? "Fled last room" : "Flee room";
  if (s::canFlee(g)) {
    fui::ButtonProps fleeButton = pill(fleeLabel, ActionFlee);
    fleeButton.icon = fui::bitmapFromIcon(icon_flee_24);
    fleeButton.gap = 8;
    screen.button(fleeButton, fleeRect);
  } else {
    // Disabled, as the phone greys it: a grey frame, the same words.
    screen.target().stroke(fleeRect, kGrey, 2, 10);
    say(screen, fleeRect, fleeLabel, Face::Large);
  }
}

void buildConfirm(toybox::Screen& screen) {
  toybox::absoluteChrome(screen);
  title(screen, "Abandon this dungeon?");
  screen.button(pill("Keep playing", ActionKeepPlaying, false), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  screen.button(pill("Find another dungeon", ActionAbandon), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  const fui::Rect area = screen.body();
  prose(screen, fui::makeRect(area.x, static_cast<int16_t>(area.y + 30), area.width, 60), "Your current run will be lost.",
        2);
}

// ---- result ---------------------------------------------------------------

namespace {

// The death screen: the whole panel. A headline, the grave with the run
// carved into its stone, what is left of the dungeon, and the way out.
void buildDeath(toybox::Screen& screen, const ResultModel& model) {
  const s::Game& g = model.game;
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  toybox::absoluteChrome(screen);

  say(screen, fui::makeRect(0, 14, w, 92), "You died", Face::Title);
  constexpr int16_t kPictureTop = 108;
  const fui::Rect picture = fui::makeRect(0, kPictureTop, w, 380);
  target.bitmap(picture, fui::bitmapFromIcon(scene_died), fui::BitmapMode::Center, kBlack);

  // Carved into the stone.
  const int16_t fx = kDiedFaceX, fy = static_cast<int16_t>(kPictureTop + kDiedFaceY), fw = kDiedFaceW;
  char line[48];
  say(screen, fui::makeRect(fx, static_cast<int16_t>(fy + 4), fw, 20), "HERE LIES", Face::Small);
  say(screen, fui::makeRect(fx, static_cast<int16_t>(fy + 24), fw, 20), "A SCOUNDREL", Face::Small);
  target.fill(fui::makeRect(static_cast<int16_t>(fx + fw / 2 - 30), static_cast<int16_t>(fy + 52), 60, 2), kBlack);
  std::snprintf(line, sizeof(line), "Room %d", g.roomNo);
  say(screen, fui::makeRect(fx, static_cast<int16_t>(fy + 60), fw, 48), line, Face::Value);
  say(screen, fui::makeRect(fx, static_cast<int16_t>(fy + 116), fw, 20), "SCORE", Face::Small);
  std::snprintf(line, sizeof(line), "%d", g.score);
  say(screen, fui::makeRect(fx, static_cast<int16_t>(fy + 136), fw, 60), line, Face::ValueBig);

  const int16_t rows = static_cast<int16_t>(picture.bottom() + 10);
  const int left = s::monstersLeft(g);
  std::snprintf(line, sizeof(line), left == 1 ? "1 monster still in the dungeon." : "%d monsters still in the dungeon.",
                left);
  say(screen, fui::makeRect(0, rows, w, 34), line, Face::Large);
  if (model.newBest) {
    std::snprintf(line, sizeof(line), "A new best score!");
  } else {
    std::snprintf(line, sizeof(line), "Best: %d", model.best);
  }
  say(screen, fui::makeRect(0, static_cast<int16_t>(rows + 34), w, 34), line, Face::Large);

  screen.setContentMarginAbsolute(fui::Insets{static_cast<int16_t>(rows + 76), toybox::kMargin, toybox::kMargin,
                                              toybox::kMargin});
  screen.button(pill("Menu", ActionDone, false), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  screen.button(pill("Start another dungeon", ActionAgain), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
}

// The survivor's screen, built like the death screen: a headline, the way
// out of the dungeon into daylight, then the run in numbers.
void buildSurvived(toybox::Screen& screen, const ResultModel& model) {
  const s::Game& g = model.game;
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  toybox::absoluteChrome(screen);

  say(screen, fui::makeRect(0, 14, w, 92), "You survived!", Face::Title);
  constexpr int16_t kPictureTop = 108;
  const fui::Rect picture = fui::makeRect(0, kPictureTop, w, 380);
  target.bitmap(picture, fui::bitmapFromIcon(scene_survived), fui::BitmapMode::Center, kBlack);

  // "SCORE 26": the label and the number on one line, centred together.
  char line[48];
  std::snprintf(line, sizeof(line), "%d", g.score);
  const int16_t top = static_cast<int16_t>(picture.bottom() + 6);
  const int16_t lw = widthOf(screen, "SCORE", Face::Small);
  const int16_t nw = widthOf(screen, line, Face::ValueBig);
  const int16_t x = static_cast<int16_t>((w - lw - 12 - nw) / 2);
  say(screen, fui::makeRect(x, top, static_cast<int16_t>(lw + 4), 60), "SCORE", Face::Small, fui::TextAlign::Left);
  say(screen, fui::makeRect(static_cast<int16_t>(x + lw + 12), top, static_cast<int16_t>(nw + 4), 60), line,
      Face::ValueBig, fui::TextAlign::Left);

  std::snprintf(line, sizeof(line), "Cleared in %d rooms, %d health left.", g.roomNo, g.hp);
  say(screen, fui::makeRect(0, static_cast<int16_t>(top + 60), w, 32), line, Face::Large);
  if (model.newBest) {
    std::snprintf(line, sizeof(line), "A new best score!");
  } else {
    std::snprintf(line, sizeof(line), "Best: %d", model.best);
  }
  say(screen, fui::makeRect(0, static_cast<int16_t>(top + 92), w, 32), line, Face::Large);

  screen.setContentMarginAbsolute(fui::Insets{static_cast<int16_t>(top + 130), toybox::kMargin, toybox::kMargin,
                                              toybox::kMargin});
  screen.button(pill("Menu", ActionDone, false), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  screen.button(pill("Start another dungeon", ActionAgain), screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
}

}  // namespace

void buildResult(toybox::Screen& screen, const ResultModel& model) {
  if (model.game.won) {
    buildSurvived(screen, model);
  } else {
    buildDeath(screen, model);
  }
}

}  // namespace scoundrelui
