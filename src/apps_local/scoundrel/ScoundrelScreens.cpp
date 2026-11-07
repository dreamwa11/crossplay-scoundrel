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

// ---- geometry -------------------------------------------------------------
//
// Two rows of two. Four cards in a row would be 104px wide each on a 480px
// panel: a 48px glyph and no room under it for the two choices a monster can
// offer. Two by two makes every card a 218px target with a 96px glyph.
constexpr int16_t kSide = toybox::kMargin;
constexpr int16_t kGap = toybox::kGutter;
constexpr int16_t kCardH = 226;
constexpr int16_t kRadius = 10;

int16_t cardW(const fui::DeviceContext& device) { return static_cast<int16_t>((device.width - kSide * 2 - kGap) / 2); }
int16_t tableTop() { return static_cast<int16_t>(toybox::kChromeHeight + kGap); }

const freeink::Icon& artFor(const uint8_t card, const bool big) {
  const int v = s::valueOf(card);
  switch (s::kindOf(card)) {
    case s::Kind::Monster:
      if (v == 11) return big ? icon_jack_96 : icon_jack_48;
      if (v == 12) return big ? icon_queen_96 : icon_queen_48;
      if (v == 13) return big ? icon_king_96 : icon_king_48;
      if (v == 14) return big ? icon_ace_96 : icon_ace_48;
      if (v == 2) return big ? icon_goblin_96 : icon_goblin_48;
      if (v <= 4) return big ? icon_wolf_96 : icon_wolf_48;
      if (v <= 6) return big ? icon_skeleton_96 : icon_skeleton_48;
      if (v <= 8) return big ? icon_orc_96 : icon_orc_48;
      return big ? icon_beast_96 : icon_beast_48;
    case s::Kind::Weapon:
      if (v <= 3) return big ? icon_dagger_96 : icon_dagger_48;
      if (v <= 5) return big ? icon_shortsword_96 : icon_shortsword_48;
      if (v <= 8) return big ? icon_longsword_96 : icon_longsword_48;
      return big ? icon_greatsword_96 : icon_greatsword_48;
    case s::Kind::Potion:
    default:
      if (v <= 3) return big ? icon_potionsmall_96 : icon_potionsmall_48;
      if (v <= 8) return big ? icon_potion_96 : icon_potion_48;
      return big ? icon_potionlarge_96 : icon_potionlarge_48;
  }
}

const freeink::Icon& markFor(const uint8_t card) {
  switch (s::kindOf(card)) {
    case s::Kind::Monster:
      return icon_skull_24;
    case s::Kind::Weapon:
      return icon_sword_24;
    default:
      return icon_flask_24;
  }
}

void icon(toybox::Screen& screen, const fui::Rect& box, const freeink::Icon& art, const bool paper = false) {
  screen.target().bitmap(box, fui::bitmapFromIcon(art), fui::BitmapMode::Center, paper ? kWhite : kBlack);
}

fui::TextStyle style(const fui::FontId font, const fui::TextAlign align = fui::TextAlign::Center) {
  fui::TextStyle t;
  t.font = font;
  t.align = align;
  return t;
}

// ---- the card face --------------------------------------------------------

// rank top-left, kind mark top-right, the glyph, the name. `body` is the area
// above the chips; a card without chips (the menu's ornament) uses all of it.
void drawFace(toybox::Screen& screen, const fui::Rect& rect, const uint8_t card) {
  auto& target = screen.target();
  target.fill(rect, kWhite, kRadius);
  target.stroke(rect, kBlack, 2, kRadius);

  target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 6), 70,
                                               toybox::kDisplayCut.lineHeight),
                                 toybox::kDisplayCut),
              s::rankLabel(card), style(toybox::kDisplayFont, fui::TextAlign::Left));
  icon(screen, fui::makeRect(static_cast<int16_t>(rect.right() - 12 - 24), static_cast<int16_t>(rect.y + 14), 24, 24),
       markFor(card));

  icon(screen,
       fui::makeRect(static_cast<int16_t>(rect.x + (rect.width - 96) / 2), static_cast<int16_t>(rect.y + 44), 96, 96),
       artFor(card, true));

  target.text(fui::makeRect(rect.x, static_cast<int16_t>(rect.y + 146), rect.width, toybox::kButtonCut.lineHeight),
              s::cardName(card), style(toybox::kSmallFont));
}

// One choice under a card. Outlined, or solid black when it is the better of
// two; a choice that would kill you carries a skull instead of its tool.
void chip(toybox::Screen& screen, const fui::Rect& box, const char* label, const freeink::Icon* art, const bool loud,
          const int16_t value) {
  fui::ButtonProps b;
  b.label = label;
  b.action = ActionCard;
  b.value = value;
  b.styles = toybox::rowStyles();
  b.state = loud ? fui::StateSelected : fui::StateNormal;
  b.text = style(toybox::kUiFont);
  if (art) b.icon = fui::bitmapFromIcon(*art);
  b.gap = 6;
  b.radius = 8;
  screen.button(b, box);
}

void drawSlot(toybox::Screen& screen, const BoardModel& model, const int slot) {
  const fui::Rect rect = slotRect(screen.device(), slot);
  const uint8_t card = model.game.room[slot];
  if (card == s::kNoCard) {
    cardart::drawCardSlot(screen, rect);
    return;
  }
  drawFace(screen, rect, card);
  if (model.game.over) return;

  // The choices, along the bottom of the card.
  const fui::Rect row = fui::makeRect(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.bottom() - 8 - 40),
                                      static_cast<int16_t>(rect.width - 16), 40);
  const s::Game& g = model.game;
  char a[12];
  char b[12];
  switch (s::kindOf(card)) {
    case s::Kind::Monster: {
      const s::Preview p = s::preview(g, card);
      const bool fistKills = p.fist >= g.hp;
      std::snprintf(a, sizeof(a), "-%d", p.fist);
      if (p.weapon < 0) {
        chip(screen, row, a, fistKills ? &icon_skull_24 : &icon_hand_24, false, encodeCard(slot, s::Mode::Fist));
        break;
      }
      const bool weaponKills = p.weapon >= g.hp;
      std::snprintf(b, sizeof(b), p.weapon ? "-%d" : "0", p.weapon);
      const int16_t half = static_cast<int16_t>((row.width - 8) / 2);
      // The weapon is the loud one unless bare hands cost no more.
      const bool fistBetter = p.fist <= p.weapon;
      chip(screen, fui::makeRect(row.x, row.y, half, row.height), a, fistKills ? &icon_skull_24 : &icon_hand_24,
           fistBetter, encodeCard(slot, s::Mode::Fist));
      chip(screen, fui::makeRect(static_cast<int16_t>(row.right() - half), row.y, half, row.height), b,
           weaponKills ? &icon_skull_24 : &icon_sword_24, !fistBetter, encodeCard(slot, s::Mode::Weapon));
      break;
    }
    case s::Kind::Weapon:
      chip(screen, row, g.weapon == s::kNoCard ? "EQUIP" : "SWAP", &icon_sword_24, false,
           encodeCard(slot, s::Mode::Auto));
      break;
    case s::Kind::Potion:
    default:
      if (g.potionUsed) {
        chip(screen, row, "WASTED", &icon_flask_24, false, encodeCard(slot, s::Mode::Auto));
      } else {
        int heal = g.maxHp - g.hp;
        if (heal > s::valueOf(card)) heal = s::valueOf(card);
        std::snprintf(a, sizeof(a), "+%d", heal);
        chip(screen, row, a, &icon_heart_24, false, encodeCard(slot, s::Mode::Auto));
      }
      break;
  }
}

// ---- the status strip -----------------------------------------------------

void drawHealth(toybox::Screen& screen, const fui::Rect& box, const s::Game& g) {
  auto& target = screen.target();
  target.stroke(box, kBlack, 2, kRadius);
  // A gauge along the bottom: what is left of the health, as area.
  const int hp = g.hp < 0 ? 0 : g.hp;
  const fui::Rect bar = fui::makeRect(static_cast<int16_t>(box.x + 12), static_cast<int16_t>(box.bottom() - 24),
                                      static_cast<int16_t>(box.width - 24), 10);
  target.stroke(bar, kBlack, 1, 3);
  const int16_t filled = static_cast<int16_t>(bar.width * hp / (g.maxHp > 0 ? g.maxHp : 1));
  if (filled > 0) target.fill(fui::makeRect(bar.x, bar.y, filled, bar.height), kBlack, 3);

  icon(screen, fui::makeRect(static_cast<int16_t>(box.x + 12), static_cast<int16_t>(box.y + 14), 32, 32),
       icon_heart_32);
  char line[16];
  std::snprintf(line, sizeof(line), "%d", hp);
  const fui::TextStyle big = style(toybox::kDisplayFont, fui::TextAlign::Left);
  const fui::Rect numberBox = toybox::inkCentred(
      fui::makeRect(static_cast<int16_t>(box.x + 50), static_cast<int16_t>(box.y + 6), 64, toybox::kDisplayCut.lineHeight),
      toybox::kDisplayCut);
  target.text(numberBox, line, big);
  const int16_t numberW = target.measureText(big.font, line, big).width;
  // The maximum beside it, small, on the same baseline: "17 /20".
  std::snprintf(line, sizeof(line), "/%d", g.maxHp);
  target.text(fui::makeRect(static_cast<int16_t>(box.x + 50 + numberW + 4), static_cast<int16_t>(box.y + 30),
                            static_cast<int16_t>(box.right() - (box.x + 50 + numberW + 4) - 6),
                            toybox::kButtonCut.lineHeight),
              line, style(toybox::kSmallFont, fui::TextAlign::Left));
}

void drawWeapon(toybox::Screen& screen, const fui::Rect& box, const s::Game& g) {
  auto& target = screen.target();
  if (g.weapon == s::kNoCard) {
    cardart::drawCardSlot(screen, box);
    icon(screen, fui::makeRect(static_cast<int16_t>(box.x + 16), static_cast<int16_t>(box.y + (box.height - 32) / 2),
                               32, 32),
         icon_hand_32);
    target.text(fui::makeRect(static_cast<int16_t>(box.x + 60), static_cast<int16_t>(box.y + 20),
                              static_cast<int16_t>(box.width - 70), toybox::kUiCut.lineHeight),
                "BARE HANDS", style(toybox::kUiFont, fui::TextAlign::Left));
    target.text(fui::makeRect(static_cast<int16_t>(box.x + 60), static_cast<int16_t>(box.y + 58),
                              static_cast<int16_t>(box.width - 70), toybox::kButtonCut.lineHeight),
                "NO WEAPON", style(toybox::kSmallFont, fui::TextAlign::Left));
    return;
  }
  target.fill(box, kWhite, kRadius);
  target.stroke(box, kBlack, 2, kRadius);
  icon(screen, fui::makeRect(static_cast<int16_t>(box.x + 10), static_cast<int16_t>(box.y + (box.height - 48) / 2), 48,
                             48),
       artFor(g.weapon, false));
  const int16_t textX = static_cast<int16_t>(box.x + 68);
  const int16_t textW = static_cast<int16_t>(box.right() - textX - 8);
  // The name on top, the value and its limit under it: "LAST KILL 8" is the
  // number the next fight is measured against.
  target.text(fui::makeRect(textX, static_cast<int16_t>(box.y + 8), textW, toybox::kButtonCut.lineHeight),
              s::cardName(g.weapon), style(toybox::kSmallFont, fui::TextAlign::Left));
  char line[24];
  std::snprintf(line, sizeof(line), "%d", s::valueOf(g.weapon));
  const fui::TextStyle big = style(toybox::kDisplayFont, fui::TextAlign::Left);
  target.text(toybox::inkCentred(fui::makeRect(textX, static_cast<int16_t>(box.y + 34), 50, toybox::kDisplayCut.lineHeight),
                                 toybox::kDisplayCut),
              line, big);
  const int16_t valueW = target.measureText(big.font, line, big).width;
  if (g.weaponLast < 0) {
    std::snprintf(line, sizeof(line), "UNUSED");
  } else {
    std::snprintf(line, sizeof(line), "LAST KILL %s", s::rankLabel(s::makeCard(s::Kind::Monster, g.weaponLast)));
  }
  const int16_t limitX = static_cast<int16_t>(textX + valueW + 12);
  target.text(fui::makeRect(limitX, static_cast<int16_t>(box.y + 50), static_cast<int16_t>(box.right() - limitX - 8),
                            toybox::kButtonCut.lineHeight),
              line, style(toybox::kSmallFont, fui::TextAlign::Left));
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
  const int16_t w = cardW(device);
  const int column = slot % 2;
  const int row = slot / 2;
  return fui::makeRect(static_cast<int16_t>(kSide + column * (w + kGap)),
                       static_cast<int16_t>(tableTop() + row * (kCardH + kGap)), w, kCardH);
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
  switch (g.note) {
    case s::Note::NewGame:
      std::snprintf(out, static_cast<size_t>(size), "TAP A CARD TO FACE IT");
      return;
    case s::Note::SlewWithWeapon:
      if (g.noteAmount > 0) {
        std::snprintf(out, static_cast<size_t>(size), "SLEW THE %s. TOOK %d.", name, g.noteAmount);
      } else {
        std::snprintf(out, static_cast<size_t>(size), "SLEW THE %s UNHARMED", name);
      }
      return;
    case s::Note::TooDull:
      std::snprintf(out, static_cast<size_t>(size), "TOO DULL. BARE HANDS: TOOK %d.", g.noteAmount);
      return;
    case s::Note::BareHands:
      std::snprintf(out, static_cast<size_t>(size), "FOUGHT THE %s. TOOK %d.", name, g.noteAmount);
      return;
    case s::Note::Equipped:
      std::snprintf(out, static_cast<size_t>(size), "EQUIPPED THE %s", name);
      return;
    case s::Note::Drank:
      if (g.noteAmount > 0) {
        std::snprintf(out, static_cast<size_t>(size), "DRANK A POTION. +%d HP.", g.noteAmount);
      } else {
        std::snprintf(out, static_cast<size_t>(size), "DRANK A POTION AT FULL HEALTH");
      }
      return;
    case s::Note::Wasted:
      std::snprintf(out, static_cast<size_t>(size), "ONE POTION PER ROOM. WASTED.");
      return;
    case s::Note::Fled:
      std::snprintf(out, static_cast<size_t>(size), "YOU FLED TO THE NEXT ROOM");
      return;
    case s::Note::CannotFlee:
      std::snprintf(out, static_cast<size_t>(size), "YOU CANNOT FLEE TWICE IN A ROW");
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
  const int16_t h = 180;
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
      auto& target = screen.target();
      target.fill(r, kWhite, kRadius);
      target.stroke(r, kBlack, 2, kRadius);
      target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(r.x + 10), static_cast<int16_t>(r.y + 4), 60,
                                                   toybox::kDisplayCut.lineHeight),
                                     toybox::kDisplayCut),
                  s::rankLabel(fan[i]), style(toybox::kDisplayFont, fui::TextAlign::Left));
      icon(screen, fui::makeRect(static_cast<int16_t>(r.right() - 34), static_cast<int16_t>(r.y + 12), 24, 24),
           markFor(fan[i]));
      icon(screen, fui::makeRect(static_cast<int16_t>(r.x + (w - 96) / 2), static_cast<int16_t>(r.y + 60), 96, 96),
           artFor(fan[i], true));
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
  const int16_t h = 180;
  const int16_t gap = 14;
  const int16_t total = static_cast<int16_t>(n * w + (n - 1) * gap);
  int16_t x = static_cast<int16_t>(area.x + (area.width - total) / 2);
  const int16_t y = static_cast<int16_t>(area.y + 280);
  for (const uint8_t c : kLessons[page].cards) {
    if (c == s::kNoCard) continue;
    const fui::Rect r = fui::makeRect(x, y, w, h);
    auto& target = screen.target();
    target.fill(r, kWhite, kRadius);
    target.stroke(r, kBlack, 2, kRadius);
    target.text(toybox::inkCentred(fui::makeRect(static_cast<int16_t>(r.x + 10), static_cast<int16_t>(r.y + 4), 60,
                                                 toybox::kDisplayCut.lineHeight),
                                   toybox::kDisplayCut),
                s::rankLabel(c), style(toybox::kDisplayFont, fui::TextAlign::Left));
    icon(screen, fui::makeRect(static_cast<int16_t>(r.right() - 34), static_cast<int16_t>(r.y + 12), 24, 24),
         markFor(c));
    icon(screen, fui::makeRect(static_cast<int16_t>(r.x + (w - 96) / 2), static_cast<int16_t>(r.y + 60), 96, 96),
         artFor(c, true));
    x = static_cast<int16_t>(x + w + gap);
  }
}

// ---- the table ------------------------------------------------------------

void buildBoard(toybox::Screen& screen, const BoardModel& model) {
  const s::Game& g = model.game;
  char title[16];
  std::snprintf(title, sizeof(title), "ROOM %d", g.roomNo);
  char left[24];
  std::snprintf(left, sizeof(left), "%d IN DECK", g.deckLen);
  chrome(screen, title, left);
  screen.insetContent(fui::Insets{0, toybox::kMargin, toybox::kMargin, toybox::kMargin});

  // The bottom control first, so nothing can crowd it out.
  if (g.over) {
    fui::ButtonProps verdict;
    verdict.label = g.won ? "YOU SURVIVED" : "YOU DIED";
    verdict.action = ActionSeeResult;
    screen.button(verdict, screen.takeBottom(toybox::kPillHeight, toybox::kGutter));
  } else {
    const fui::Rect strip = screen.takeBottom(toybox::kPillHeight, toybox::kGutter);
    if (s::canFlee(g)) {
      fui::ButtonProps fleeButton;
      fleeButton.label = "FLEE THIS ROOM";
      fleeButton.action = ActionFlee;
      fleeButton.styles = toybox::rowStyles();
      screen.button(fleeButton, strip);
    } else {
      // Not a control while it cannot be used: a caption in a grey frame, so
      // it reads as spent rather than as a button that ignores you.
      screen.target().stroke(strip, fui::Paint::dither(fui::Color::DarkGray), 2);
      screen.target().text(toybox::inkCentred(strip, toybox::kButtonCut),
                           g.fledLast ? "FLED LAST ROOM: FACE THIS ONE" : "FLEE ONLY FROM A FULL ROOM",
                           style(toybox::kSmallFont));
    }
  }

  for (int slot = 0; slot < s::kRoomSize; ++slot) drawSlot(screen, model, slot);

  const fui::DeviceContext device = screen.device();
  const fui::Rect lastCard = slotRect(device, 3);
  const int16_t stripTop = static_cast<int16_t>(lastCard.bottom() + kGap);
  const int16_t stripH = 92;
  const int16_t healthW = 168;
  drawHealth(screen, fui::makeRect(kSide, stripTop, healthW, stripH), g);
  drawWeapon(screen,
             fui::makeRect(static_cast<int16_t>(kSide + healthW + kGap), stripTop,
                           static_cast<int16_t>(device.width - kSide * 2 - healthW - kGap), stripH),
             g);

  char note[64];
  noteText(g, note, sizeof(note));
  const fui::Rect content = screen.contentRect();
  const int16_t noteTop = static_cast<int16_t>(stripTop + stripH);
  const int16_t noteH = static_cast<int16_t>(content.bottom() - noteTop);
  if (noteH > 10) {
    screen.target().text(toybox::inkCentred(fui::makeRect(kSide, noteTop, static_cast<int16_t>(device.width - kSide * 2),
                                                          noteH),
                                            toybox::kButtonCut),
                         note, style(toybox::kSmallFont));
  }
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
