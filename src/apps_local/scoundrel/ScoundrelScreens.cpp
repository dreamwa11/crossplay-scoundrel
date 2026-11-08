#include "ScoundrelScreens.h"

#include <FreeInkUIIcon.h>

#include <cstdio>
#include <cstring>

#include "../cards/CardArt.h"
#include "../ui/ToyboxFormat.h"
#include "ScoundrelArt.h"

namespace scoundrelui {

namespace {

namespace s = scoundrel;

const fui::Paint kBlack = fui::Paint::solid(fui::Color::Black);
const fui::Paint kWhite = fui::Paint::solid(fui::Color::White);
constexpr int16_t kRadius = 8;

// ---- geometry -------------------------------------------------------------
//
// The phone version's arrangement, top to bottom, on a 480x800 panel:
//
//   top bar      cards left / monsters left, ROOM n, the menu button
//   the scene    a doorway in a brick wall
//   the room     four cards in a row, their choices underneath
//   message      what just happened
//   banner       SCOUNDREL
//   the desk     DECK, HEALTH, WEAPON
//   buttons      NEW DUNGEON, FLEE ROOM
constexpr int16_t kTopBarH = 58;
constexpr int16_t kSceneBottom = 238;
constexpr int16_t kRoomTop = 246;
constexpr int16_t kCardW = 110;
constexpr int16_t kCardH = 150;
constexpr int16_t kRoomGap = 8;
constexpr int16_t kChipTop = kRoomTop + kCardH + 6;
constexpr int16_t kChipH = 40;
constexpr int16_t kNoteTop = kChipTop + kChipH + 4;
constexpr int16_t kNoteH = 30;
constexpr int16_t kBannerTop = kNoteTop + kNoteH + 4;
constexpr int16_t kBannerH = 42;
constexpr int16_t kDeskTop = kBannerTop + kBannerH + 10;
constexpr int16_t kDeskW = 104;
constexpr int16_t kDeskH = 160;
constexpr int16_t kButtonsTop = 740;

int16_t roomLeft(const fui::DeviceContext& device) {
  return static_cast<int16_t>((device.width - (kCardW * 4 + kRoomGap * 3)) / 2);
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

fui::TextStyle style(const fui::FontId font, const fui::TextAlign align = fui::TextAlign::Center,
                     const bool paper = false) {
  fui::TextStyle t;
  t.font = font;
  t.align = align;
  if (paper) t.color = fui::Color::White;
  return t;
}

fui::Rect centred(const int16_t cx, const int16_t y, const int16_t w, const int16_t h) {
  return fui::makeRect(static_cast<int16_t>(cx - w / 2), y, w, h);
}

// ---- the card face --------------------------------------------------------
//
// Kind mark top-left, value top-right, the picture, the name along the bottom.
// `art` is 64 for the room and the weapon slot, 96 for the menus. `nameFont` is
// whichever slot holds the smallest cut on the calling screen.
void drawFace(toybox::Screen& screen, const fui::Rect& rect, const uint8_t card, const int art,
              const fui::FontId nameFont, const int16_t nameLine) {
  auto& target = screen.target();
  target.fill(rect, kWhite, kRadius);
  target.stroke(rect, kBlack, 2, kRadius);

  icon(screen, fui::makeRect(static_cast<int16_t>(rect.x + 7), static_cast<int16_t>(rect.y + 8), 24, 24),
       markFor(card));
  target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(rect.right() - 8 - 64),
                                               static_cast<int16_t>(rect.y + 2), 64, toybox::kDisplayCut.lineHeight),
                                 toybox::kDisplayCut),
              s::rankLabel(card), style(toybox::kDisplayFont, fui::TextAlign::Right));

  const int16_t nameTop = static_cast<int16_t>(rect.bottom() - nameLine - 4);
  const int16_t artTop = static_cast<int16_t>(rect.y + 36 + (nameTop - rect.y - 36 - art) / 2);
  icon(screen, centred(static_cast<int16_t>(rect.x + rect.width / 2), artTop, static_cast<int16_t>(art),
                       static_cast<int16_t>(art)),
       artFor(card, art));
  target.text(fui::makeRect(static_cast<int16_t>(rect.x + 2), nameTop, static_cast<int16_t>(rect.width - 4), nameLine),
              s::cardName(card), style(nameFont));
}

// One choice under a room card: an outlined chip, or solid black when it is the
// better of two; a choice that would kill you carries a skull instead of its
// tool. The button is registered bare and its contents drawn here, packed
// tighter than the button's own layout allows: a chip is 53px wide and has to
// hold a 24px mark and a two-digit number.
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

  auto& target = screen.target();
  const fui::TextStyle t = style(toybox::kUiFont, fui::TextAlign::Left, loud);
  const int16_t tw = target.measureText(t.font, label, t).width;
  const int16_t iw = art ? 24 : 0;
  const int16_t gap = art ? 2 : 0;
  const int16_t x = static_cast<int16_t>(box.x + (box.width - iw - gap - tw) / 2);
  if (art) icon(screen, fui::makeRect(x, static_cast<int16_t>(box.y + (box.height - 24) / 2), 24, 24), *art, loud);
  target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(x + iw + gap), box.y, static_cast<int16_t>(tw + 4),
                                               box.height),
                                 toybox::kUiCut),
              label, t);
}

// The weapon choice when there is no weapon, or it is too dull: shown, not
// offered, like the phone's greyed-out sword.
void spentChip(toybox::Screen& screen, const fui::Rect& box) {
  screen.target().stroke(box, fui::Paint::dither(fui::Color::DarkGray), 2, 8);
  screen.target().bitmap(centred(static_cast<int16_t>(box.x + box.width / 2 - 8),
                                 static_cast<int16_t>(box.y + (box.height - 24) / 2), 24, 24),
                         fui::bitmapFromIcon(icon_sword_24), fui::BitmapMode::Center,
                         fui::Paint::dither(fui::Color::DarkGray));
  screen.target().text(fui::makeRect(static_cast<int16_t>(box.x + box.width / 2 + 4), box.y,
                                     static_cast<int16_t>(box.width / 2 - 6), box.height),
                       "-", style(toybox::kUiFont, fui::TextAlign::Left));
}

void drawRoomCard(toybox::Screen& screen, const BoardModel& model, const int slot) {
  const fui::Rect rect = slotRect(screen.device(), slot);
  const uint8_t card = model.game.room[slot];
  if (card == s::kNoCard) {
    cardart::drawCardSlot(screen, rect);
    return;
  }
  drawFace(screen, rect, card, 64, toybox::kSmallFont, toybox::kTileCut.lineHeight);
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
      chip(screen, row, g.weapon == s::kNoCard ? "EQUIP" : "SWAP", nullptr, false, encodeCard(slot, s::Mode::Auto));
      break;
    case s::Kind::Potion:
    default:
      if (g.potionUsed) {
        chip(screen, row, "WASTED", nullptr, false, encodeCard(slot, s::Mode::Auto));
      } else {
        int heal = g.maxHp - g.hp;
        if (heal > s::valueOf(card)) heal = s::valueOf(card);
        std::snprintf(a, sizeof(a), "+%d", heal);
        chip(screen, row, a, &icon_heart_24, false, encodeCard(slot, s::Mode::Auto));
      }
      break;
  }
}

// ---- top bar, scene, banner -------------------------------------------------

void topBar(toybox::Screen& screen, const s::Game& g) {
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  char line[16];
  int16_t x = 10;
  const int16_t midY = kTopBarH / 2;
  auto counter = [&](const freeink::Icon& art, const int n) {
    icon(screen, fui::makeRect(x, static_cast<int16_t>(midY - 12), 24, 24), art);
    std::snprintf(line, sizeof(line), "%d", n);
    const fui::TextStyle t = style(toybox::kUiFont, fui::TextAlign::Left);
    const int16_t tw = target.measureText(t.font, line, t).width;
    target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(x + 26), 0, static_cast<int16_t>(tw + 4), kTopBarH),
                                   toybox::kUiCut),
                line, t);
    x = static_cast<int16_t>(x + 26 + tw + 10);
  };
  counter(icon_deck_24, g.deckLen + s::roomCount(g));
  counter(icon_skull_24, s::monstersLeft(g));

  // The room's name in whatever is left between the counters and the menu.
  std::snprintf(line, sizeof(line), "ROOM %d", g.roomNo);
  const int16_t right = static_cast<int16_t>(w - 12 - 46 - 6);
  target.text(toybox::inkCentred(fui::makeRect(x, 0, static_cast<int16_t>(right - x), kTopBarH), toybox::kDisplayCut),
              line, style(toybox::kDisplayFont));

  fui::ButtonProps menu;
  menu.icon = fui::bitmapFromIcon(icon_menu_24);
  menu.action = ActionMenu;
  menu.styles = toybox::rowStyles();
  menu.radius = 8;
  screen.button(menu, fui::makeRect(static_cast<int16_t>(w - 12 - 46), static_cast<int16_t>(midY - 23), 46, 46));
  target.fill(fui::makeRect(0, kTopBarH, w, 2), kBlack);
}

// The doorway in a brick wall, as on the phone. Drawn once per repaint and
// never changes, so it costs no ghosting.
void scene(toybox::Screen& screen) {
  auto& target = screen.target();
  const int16_t w = screen.device().width;
  const int16_t top = kTopBarH + 2;
  target.fill(fui::makeRect(0, top, w, static_cast<int16_t>(kSceneBottom - top)),
              fui::Paint::dither(fui::Color::LightGray));
  // Mortar: white courses, the bricks offset every other row.
  constexpr int16_t kCourse = 22;
  constexpr int16_t kBrick = 56;
  int row = 0;
  for (int16_t y = static_cast<int16_t>(top + kCourse); y < kSceneBottom; y = static_cast<int16_t>(y + kCourse), ++row) {
    target.fill(fui::makeRect(0, y, w, 2), kWhite);
    const int16_t offset = (row % 2) ? kBrick / 2 : 0;
    for (int16_t x = offset; x < w; x = static_cast<int16_t>(x + kBrick)) {
      target.fill(fui::makeRect(x, static_cast<int16_t>(y - kCourse + 2), 2, static_cast<int16_t>(kCourse - 2)), kWhite);
    }
  }
  // The doorway: a dark arch with a stone frame, its foot behind the room.
  const int16_t doorW = 168;
  const fui::Rect frame = centred(static_cast<int16_t>(w / 2), static_cast<int16_t>(top + 18),
                                  static_cast<int16_t>(doorW + 28), 300);
  target.fill(frame, kWhite, static_cast<uint8_t>((doorW + 28) / 2));
  target.stroke(frame, kBlack, 3, static_cast<uint8_t>((doorW + 28) / 2));
  const fui::Rect door = centred(static_cast<int16_t>(w / 2), static_cast<int16_t>(top + 32), doorW, 300);
  target.fill(door, fui::Paint::dither(fui::Color::DarkGray), static_cast<uint8_t>(doorW / 2));
  target.stroke(door, kBlack, 3, static_cast<uint8_t>(doorW / 2));
  // The floor the room is dealt onto.
  target.fill(fui::makeRect(0, kSceneBottom, w, static_cast<int16_t>(kRoomTop + kCardH + 4 - kSceneBottom)), kWhite);
  target.fill(fui::makeRect(0, kSceneBottom, w, 2), kBlack);
}

void banner(toybox::Screen& screen) {
  const int16_t w = screen.device().width;
  const fui::Rect band = fui::makeRect(0, kBannerTop, w, kBannerH);
  screen.target().fill(band, kBlack);
  screen.target().text(toybox::inkCentred(band, toybox::kDisplayCut), "SCOUNDREL",
                       style(toybox::kDisplayFont, fui::TextAlign::Center, true));
}

// ---- the desk ---------------------------------------------------------------

fui::Rect deskRect(const fui::DeviceContext& device, const int i) {
  const int16_t gap = static_cast<int16_t>((device.width - kDeskW * 3) / 4);
  return fui::makeRect(static_cast<int16_t>(gap + i * (kDeskW + gap)), kDeskTop, kDeskW, kDeskH);
}

void deskLabel(toybox::Screen& screen, const fui::Rect& slot, const char* label) {
  screen.target().text(fui::makeRect(static_cast<int16_t>(slot.x - 10), static_cast<int16_t>(slot.bottom() + 22),
                                     static_cast<int16_t>(slot.width + 20), toybox::kTileCut.lineHeight),
                       label, style(toybox::kSmallFont));
}

// A white pill hanging off the bottom of a desk card: the deck's count, the
// weapon's limit.
void badge(toybox::Screen& screen, const fui::Rect& slot, const char* text, const int16_t width) {
  const fui::Rect pill = centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.bottom() - 22),
                                 width, 40);
  screen.target().fill(pill, kWhite, 12);
  screen.target().stroke(pill, kBlack, 2, 12);
  screen.target().text(toybox::inkCentred(pill, toybox::kUiCut), text, style(toybox::kUiFont));
}

void drawDeck(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  auto& target = screen.target();
  if (g.deckLen == 0) {
    cardart::drawCardSlot(screen, slot);
  } else {
    // A little stack behind the top card while there is one.
    if (g.deckLen > 1) {
      const fui::Rect under = fui::makeRect(static_cast<int16_t>(slot.x - 4), static_cast<int16_t>(slot.y + 4),
                                            slot.width, slot.height);
      target.fill(under, kWhite, kRadius);
      target.stroke(under, kBlack, 2, kRadius);
    }
    target.fill(slot, kWhite, kRadius);
    target.stroke(slot, kBlack, 2, kRadius);
    const fui::Rect inner = fui::makeRect(static_cast<int16_t>(slot.x + 7), static_cast<int16_t>(slot.y + 7),
                                          static_cast<int16_t>(slot.width - 14), static_cast<int16_t>(slot.height - 14));
    target.fill(inner, fui::Paint::dither(fui::Color::DarkGray), kRadius / 2);
    target.stroke(inner, kBlack, 2, kRadius / 2);
    const fui::Rect halo = centred(static_cast<int16_t>(slot.x + slot.width / 2),
                                   static_cast<int16_t>(slot.y + slot.height / 2 - 30), 52, 52);
    target.fill(halo, kWhite, 26);
    target.stroke(halo, kBlack, 2, 26);
    icon(screen, centred(static_cast<int16_t>(halo.x + 26), static_cast<int16_t>(halo.y + 10), 32, 32), icon_skull_32);
  }
  char line[8];
  std::snprintf(line, sizeof(line), "%d", g.deckLen);
  badge(screen, slot, line, 56);
  deskLabel(screen, slot, "DECK");
}

void drawHero(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  auto& target = screen.target();
  target.fill(slot, kWhite, kRadius);
  target.stroke(slot, kBlack, 4, kRadius);
  icon(screen, centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.y + 6), 96, 96),
       icon_hero_96);
  // "17/20": the health large, the maximum small, sharing a baseline.
  const int hp = g.hp < 0 ? 0 : g.hp;
  char big[8];
  char small[8];
  std::snprintf(big, sizeof(big), "%d", hp);
  std::snprintf(small, sizeof(small), "/%d", g.maxHp);
  const fui::TextStyle bigStyle = style(toybox::kDisplayFont, fui::TextAlign::Left);
  const fui::TextStyle smallStyle = style(toybox::kSmallFont, fui::TextAlign::Left);
  const int16_t bw = target.measureText(bigStyle.font, big, bigStyle).width;
  const int16_t sw = target.measureText(smallStyle.font, small, smallStyle).width;
  const int16_t x = static_cast<int16_t>(slot.x + (slot.width - bw - sw - 2) / 2);
  const int16_t y = static_cast<int16_t>(slot.y + 108);
  target.text(toybox::inkCentred(fui::makeRect(x, y, static_cast<int16_t>(bw + 2), 40), toybox::kDisplayCut), big,
              bigStyle);
  target.text(fui::makeRect(static_cast<int16_t>(x + bw + 2), static_cast<int16_t>(y + 18), static_cast<int16_t>(sw + 4),
                            toybox::kTileCut.lineHeight),
              small, smallStyle);
  deskLabel(screen, slot, "HEALTH");
}

void drawWeaponSlot(toybox::Screen& screen, const fui::Rect& slot, const s::Game& g) {
  if (g.weapon == s::kNoCard) {
    cardart::drawCardSlot(screen, slot);
    icon(screen, centred(static_cast<int16_t>(slot.x + slot.width / 2), static_cast<int16_t>(slot.y + 44), 32, 32),
         icon_hand_32);
    screen.target().text(fui::makeRect(slot.x, static_cast<int16_t>(slot.y + 84), slot.width, toybox::kTileCut.lineHeight),
                         "NO WEAPON", style(toybox::kSmallFont));
  } else {
    // The kills stack behind it, one sliver each, as on the phone.
    const int shown = g.slainLen > 4 ? 4 : g.slainLen;
    for (int i = shown; i > 0; --i) {
      const fui::Rect under = fui::makeRect(static_cast<int16_t>(slot.x + 3 * i), static_cast<int16_t>(slot.y - 3 * i),
                                            slot.width, slot.height);
      screen.target().fill(under, kWhite, kRadius);
      screen.target().stroke(under, kBlack, 2, kRadius);
    }
    drawFace(screen, slot, g.weapon, 64, toybox::kSmallFont, toybox::kTileCut.lineHeight);
    if (g.weaponLast >= 0) {
      char line[12];
      std::snprintf(line, sizeof(line), "MAX %s", s::rankLabel(s::makeCard(s::Kind::Monster, g.weaponLast)));
      badge(screen, slot, line, 100);
    }
  }
  deskLabel(screen, slot, "WEAPON");
}

void chrome(toybox::Screen& screen, const char* title, const char* right) {
  fui::HeaderProps header;
  header.title = title;
  header.rightLabel = right;
  header.subtitleText = fui::TextStyle{};
  header.subtitleText.font = toybox::kUiFont;
  header.subtitleText.color = fui::Color::White;
  header.subtitleText.align = fui::TextAlign::Right;
  header.borderEdges = fui::EdgesNone;
  toybox::absoluteChrome(screen);
  toybox::headerBand(screen, header);
}

}  // namespace

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
  switch (g.note) {
    case s::Note::NewGame:
      std::snprintf(out, n, "TAP A CARD TO FACE IT");
      return;
    case s::Note::SlewWithWeapon:
      if (g.noteAmount > 0) {
        std::snprintf(out, n, "SLEW THE %s: -%d", name, g.noteAmount);
      } else {
        std::snprintf(out, n, "SLEW THE %s UNHARMED", name);
      }
      return;
    case s::Note::TooDull:
      std::snprintf(out, n, "TOO DULL. BARE HANDS: -%d", g.noteAmount);
      return;
    case s::Note::BareHands:
      std::snprintf(out, n, "FOUGHT THE %s: -%d", name, g.noteAmount);
      return;
    case s::Note::Equipped:
      std::snprintf(out, n, "EQUIPPED THE %s", name);
      return;
    case s::Note::Drank:
      if (g.noteAmount > 0) {
        std::snprintf(out, n, "DRANK A POTION: +%d", g.noteAmount);
      } else {
        std::snprintf(out, n, "ALREADY AT FULL HEALTH");
      }
      return;
    case s::Note::Wasted:
      std::snprintf(out, n, "ONE POTION PER ROOM: WASTED");
      return;
    case s::Note::Fled:
      std::snprintf(out, n, "YOU FLED TO THE NEXT ROOM");
      return;
    case s::Note::CannotFlee:
      std::snprintf(out, n, "NOT TWICE IN A ROW");
      return;
    case s::Note::None:
    default:
      if (size > 0) out[0] = '\0';
      return;
  }
}

// ---- menu -----------------------------------------------------------------

void buildMenu(toybox::Screen& screen, const MenuModel& model) {
  chrome(screen, "SCOUNDREL", nullptr);
  screen.insetContent(fui::Insets{toybox::kGutter * 3, toybox::kMargin, toybox::kMargin, toybox::kMargin});

  char record[64];
  if (model.hasBest) {
    std::snprintf(record, sizeof(record), "%d PLAYED   %d SURVIVED   BEST %d", model.played, model.survived, model.best);
  } else {
    std::snprintf(record, sizeof(record), "%d PLAYED   %d SURVIVED", model.played, model.survived);
  }
  const fui::Rect line = screen.takeTop(26);
  screen.target().text(line, record, style(toybox::kTileFont, fui::TextAlign::Left));
  screen.target().fill(fui::makeRect(line.x, static_cast<int16_t>(line.bottom() + 6), line.width, toybox::kRule),
                       kBlack);

  static char healthLabel[24];
  std::snprintf(healthLabel, sizeof(healthLabel), "%d HP", model.maxHp);
  fui::ListItem rows[static_cast<int>(MenuRow::Count)] = {};
  int16_t count = 0;
  int16_t selected = 0;
  if (model.canContinue) {
    rows[count].label = "CONTINUE";
    rows[count].actionValue = static_cast<int16_t>(MenuRow::Continue);
    ++count;
  }
  rows[count].label = "NEW DUNGEON";
  rows[count].actionValue = static_cast<int16_t>(MenuRow::NewGame);
  ++count;
  rows[count].label = "HOW TO PLAY";
  rows[count].actionValue = static_cast<int16_t>(MenuRow::HowTo);
  ++count;
  rows[count].label = "STARTING HEALTH";
  rows[count].value = healthLabel;
  rows[count].actionValue = static_cast<int16_t>(MenuRow::Health);
  ++count;

  fui::ListProps list;
  list.items = rows;
  list.count = static_cast<uint16_t>(count);
  list.selectedIndex = selected;
  list.action = ActionMenuRow;
  const int16_t listHeight =
      static_cast<int16_t>(count * toybox::kRowHeight + (count - 1) * toybox::kGutter / 2 + toybox::kGutter);
  const fui::Rect content = screen.contentRect();
  const int16_t listTop = static_cast<int16_t>(content.bottom() - listHeight);
  screen.list(list, listHeight, fui::LayoutAnchor::Bottom);

  // The ornament: one card of each kind, fanned.
  const int16_t areaTop = static_cast<int16_t>(line.bottom() + 6 + toybox::kRule);
  const int16_t room = static_cast<int16_t>(listTop - areaTop);
  const int16_t w = 150;
  const int16_t h = 200;
  if (room >= h + 20) {
    const int16_t top = static_cast<int16_t>(areaTop + (room - h) / 2);
    const int16_t centre = static_cast<int16_t>(content.x + content.width / 2);
    const uint8_t fan[3] = {s::makeCard(s::Kind::Weapon, 7), s::makeCard(s::Kind::Monster, 14, 1),
                            s::makeCard(s::Kind::Potion, 9)};
    const int16_t offsets[3] = {-140, 0, 140};
    const int16_t drops[3] = {14, 0, 14};
    for (int i : {0, 2, 1}) {
      const fui::Rect r = fui::makeRect(static_cast<int16_t>(centre - w / 2 + offsets[i]),
                                        static_cast<int16_t>(top + drops[i]), w, h);
      drawFace(screen, r, fan[i], 96, toybox::kSmallFont, toybox::kButtonCut.lineHeight);
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
    {"THE DUNGEON IS A DECK OF 44 CARDS: MONSTERS, WEAPONS AND POTIONS. CLEAR IT ALL TO SURVIVE.",
     {0x40 | 0x0C, 0x10 | 0x05, 0x20 | 0x06}},
    {"EACH ROOM DEALS FOUR CARDS. FACE THREE OF THEM AND THE ROOM REFILLS AROUND THE ONE YOU LEFT.",
     {0x00 | 0x02, 0x10 | 0x01, 0x20 | 0x03}},
    {"A MONSTER HITS FOR ITS VALUE. FIGHT BARE-HANDED AND TAKE IT ALL, OR USE YOUR WEAPON AND TAKE ONLY THE "
     "DIFFERENCE.",
     {0x00 | 0x07, 0x10 | 0x03, s::kNoCard}},
    {"A WEAPON BLUNTS: IT CAN ONLY HIT MONSTERS NO STRONGER THAN ITS LAST KILL. FIGHT A BIG ONE BARE-HANDED TO "
     "KEEP IT SHARP.",
     {0x00 | 0x09, 0x10 | 0x07, 0x40 | 0x0B}},
    {"POTIONS HEAL, UP TO YOUR STARTING HEALTH. ONLY ONE PER ROOM COUNTS; A SECOND IS WASTED.",
     {0x20 | 0x01, 0x20 | 0x07, s::kNoCard}},
    {"YOU MAY FLEE A FULL ROOM. ITS CARDS GO TO THE BOTTOM OF THE DECK. NEVER TWICE IN A ROW.",
     {s::kNoCard, s::kNoCard, s::kNoCard}},
};

}  // namespace

int howToPages() { return static_cast<int>(sizeof(kLessons) / sizeof(kLessons[0])); }

void buildHowTo(toybox::Screen& screen, const HowToModel& model) {
  const int pages = howToPages();
  const int page = model.page < 0 ? 0 : (model.page >= pages ? pages - 1 : model.page);
  char progress[toybox::kOfCounterChars];
  std::snprintf(progress, sizeof(progress), "%d OF %d", page + 1, pages);
  chrome(screen, "HOW TO PLAY", progress);
  screen.insetContent(fui::Insets{toybox::kGutter * 3, toybox::kMargin, toybox::kMargin, toybox::kMargin});

  fui::ButtonProps next;
  next.label = page + 1 < pages ? "NEXT" : "GOT IT";
  next.action = ActionHowToNext;
  screen.button(next, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));

  const fui::Rect area = screen.body();
  fui::TextStyle body = style(toybox::kBodyFont);
  body.maxLines = 6;
  screen.target().text(fui::makeRect(area.x, area.y, area.width, 260), kLessons[page].text, body);

  int n = 0;
  for (const uint8_t c : kLessons[page].cards) n += c != s::kNoCard;
  if (n == 0) return;
  const int16_t w = 132;
  const int16_t h = 200;
  const int16_t gap = 14;
  const int16_t total = static_cast<int16_t>(n * w + (n - 1) * gap);
  int16_t x = static_cast<int16_t>(area.x + (area.width - total) / 2);
  const int16_t y = static_cast<int16_t>(area.y + 280);
  for (const uint8_t c : kLessons[page].cards) {
    if (c == s::kNoCard) continue;
    drawFace(screen, fui::makeRect(x, y, w, h), c, 96, toybox::kSmallFont, toybox::kButtonCut.lineHeight);
    x = static_cast<int16_t>(x + w + gap);
  }
}

// ---- the table ------------------------------------------------------------

void buildBoard(toybox::Screen& screen, const BoardModel& model) {
  const s::Game& g = model.game;
  const fui::DeviceContext device = screen.device();
  toybox::absoluteChrome(screen);

  scene(screen);
  topBar(screen, g);
  for (int slot = 0; slot < s::kRoomSize; ++slot) drawRoomCard(screen, model, slot);

  char note[48];
  noteText(g, note, sizeof(note));
  screen.target().text(toybox::inkCentred(fui::makeRect(8, kNoteTop, static_cast<int16_t>(device.width - 16), kNoteH),
                                          toybox::kUiCut),
                       note, style(toybox::kUiFont));
  banner(screen);

  drawDeck(screen, deskRect(device, 0), g);
  drawHero(screen, deskRect(device, 1), g);
  drawWeaponSlot(screen, deskRect(device, 2), g);

  const int16_t side = 12;
  const fui::Rect strip =
      fui::makeRect(side, kButtonsTop, static_cast<int16_t>(device.width - side * 2), toybox::kPillHeight);
  if (g.over) {
    fui::ButtonProps verdict;
    verdict.label = g.won ? "YOU SURVIVED" : "YOU DIED";
    verdict.action = ActionSeeResult;
    screen.button(verdict, strip);
    return;
  }
  const int16_t half = static_cast<int16_t>((strip.width - side) / 2);
  fui::ButtonProps fresh;
  fresh.label = "NEW GAME";
  fresh.action = ActionNewGame;
  fresh.styles = toybox::rowStyles();
  screen.button(fresh, fui::makeRect(strip.x, strip.y, half, strip.height));

  const fui::Rect fleeRect = fui::makeRect(static_cast<int16_t>(strip.right() - half), strip.y, half, strip.height);
  if (s::canFlee(g)) {
    fui::ButtonProps fleeButton;
    fleeButton.label = "FLEE ROOM";
    fleeButton.icon = fui::bitmapFromIcon(icon_flee_24);
    fleeButton.action = ActionFlee;
    fleeButton.gap = 6;
    screen.button(fleeButton, fleeRect);
  } else {
    // Spent rather than hidden: a grey frame, and the reason in place of the verb.
    screen.target().stroke(fleeRect, fui::Paint::dither(fui::Color::DarkGray), 2);
    screen.target().text(toybox::inkCentred(fleeRect, toybox::kUiCut), g.fledLast ? "JUST FLED" : "NO ESCAPE",
                         style(toybox::kUiFont));
  }
}

void buildConfirm(toybox::Screen& screen) {
  chrome(screen, "NEW DUNGEON?", nullptr);
  screen.insetContent(fui::Insets{toybox::kGutter * 3, toybox::kMargin, toybox::kMargin, toybox::kMargin});
  fui::ButtonProps keep;
  keep.label = "KEEP PLAYING";
  keep.action = ActionKeepPlaying;
  keep.styles = toybox::rowStyles();
  screen.button(keep, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  fui::ButtonProps abandon;
  abandon.label = "FIND ANOTHER DUNGEON";
  abandon.action = ActionAbandon;
  screen.button(abandon, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  const fui::Rect area = screen.body();
  fui::TextStyle body = style(toybox::kBodyFont);
  body.maxLines = 3;
  screen.target().text(fui::makeRect(area.x, static_cast<int16_t>(area.y + 40), area.width, 120),
                       "ABANDON THIS DUNGEON? YOUR CURRENT RUN WILL BE LOST.", body);
}

// ---- result ---------------------------------------------------------------

void buildResult(toybox::Screen& screen, const ResultModel& model) {
  const s::Game& g = model.game;
  chrome(screen, g.won ? "SURVIVED" : "DEAD", nullptr);
  screen.insetContent(fui::Insets{toybox::kGutter * 3, toybox::kMargin, toybox::kMargin, toybox::kMargin});

  fui::ButtonProps done;
  done.label = "DONE";
  done.action = ActionDone;
  done.styles = toybox::rowStyles();
  // The body slot holds the huge cut on this screen, so the labels name theirs.
  done.text = toybox::buttonText(toybox::themeTokens());
  screen.button(done, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  fui::ButtonProps again;
  again.label = "NEW DUNGEON";
  again.action = ActionAgain;
  again.text = toybox::buttonText(toybox::themeTokens());
  screen.button(again, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));

  const fui::Rect area = screen.body();
  auto& target = screen.target();
  // This screen binds the huge cut to the body slot (see ScoundrelActivity's
  // resultFaces), so the score is the one thing on it read from across a room.
  const fui::TextStyle small = style(toybox::kSmallFont);
  target.text(fui::makeRect(area.x, area.y, area.width, toybox::kButtonCut.lineHeight), "SCORE", small);
  char line[64];
  std::snprintf(line, sizeof(line), "%d", g.score);
  target.text(toybox::inkCentred(fui::makeRect(area.x, static_cast<int16_t>(area.y + 30), area.width,
                                               toybox::kHugeCut.lineHeight),
                                 toybox::kHugeCut),
              line, style(toybox::kBodyFont));
  const int16_t ruleY = static_cast<int16_t>(area.y + 30 + toybox::kHugeCut.lineHeight + 16);
  target.fill(fui::makeRect(area.x, ruleY, area.width, toybox::kRule), kBlack);

  int16_t y = static_cast<int16_t>(ruleY + 24);
  const int16_t lineH = 40;
  auto say = [&](const char* text) {
    target.text(fui::makeRect(area.x, y, area.width, lineH), text, small);
    y = static_cast<int16_t>(y + lineH);
  };
  if (g.won) {
    std::snprintf(line, sizeof(line), "CLEARED THE DUNGEON IN %d ROOMS", g.roomNo);
    say(line);
    std::snprintf(line, sizeof(line), "%d HEALTH LEFT", g.hp);
    say(line);
  } else {
    std::snprintf(line, sizeof(line), "FELL IN ROOM %d", g.roomNo);
    say(line);
    std::snprintf(line, sizeof(line), "%d MONSTERS LEFT", s::monstersLeft(g));
    say(line);
  }
  if (model.newBest) {
    say("A NEW BEST SCORE");
  } else {
    std::snprintf(line, sizeof(line), "BEST %d", model.best);
    say(line);
  }
}

}  // namespace scoundrelui
