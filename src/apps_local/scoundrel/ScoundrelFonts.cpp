#include "ScoundrelFonts.h"

#include <EpdFont.h>
#include <EpdFontFamily.h>
#include <GfxRenderer.h>

#include "fonts/scnd_body.h"
#include "fonts/scnd_large.h"
#include "fonts/scnd_logo.h"
#include "fonts/scnd_small.h"
#include "fonts/scnd_value.h"
#include "fonts/scnd_value_big.h"

namespace scoundrelfonts {
namespace {

EpdFont small(&scnd_small);
EpdFont body(&scnd_body);
EpdFont large(&scnd_large);
EpdFont value(&scnd_value);
EpdFont valueBig(&scnd_value_big);
EpdFont logo(&scnd_logo);
EpdFontFamily smallFamily(&small);
EpdFontFamily bodyFamily(&body);
EpdFontFamily largeFamily(&large);
EpdFontFamily valueFamily(&value);
EpdFontFamily valueBigFamily(&valueBig);
EpdFontFamily logoFamily(&logo);
bool registered = false;

}  // namespace

void ensure(GfxRenderer& renderer) {
  if (registered) return;
  renderer.insertFont(kSmallId, smallFamily);
  renderer.insertFont(kBodyId, bodyFamily);
  renderer.insertFont(kLargeId, largeFamily);
  renderer.insertFont(kValueId, valueFamily);
  renderer.insertFont(kValueBigId, valueBigFamily);
  renderer.insertFont(kLogoId, logoFamily);
  registered = true;
}

}  // namespace scoundrelfonts
