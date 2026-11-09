#pragma once

// Scoundrel's own faces, the phone version's: Astloch for the logo, Scoundrel Gothic
// for values, Space Grotesk for everything else. Registered with the renderer
// the first time the game opens; see fonts/ for the cuts.

class GfxRenderer;

namespace scoundrelfonts {

constexpr int kSmallId = 0x5C0D'0001;     // Space Grotesk 600, ~11px caps
constexpr int kBodyId = 0x5C0D'0002;      // Space Grotesk 500, ~14px caps
constexpr int kLargeId = 0x5C0D'0003;     // Space Grotesk 500, ~17px caps
constexpr int kValueId = 0x5C0D'0004;     // Scoundrel Gothic, ~28px caps
constexpr int kValueBigId = 0x5C0D'0005;  // Scoundrel Gothic, ~41px caps
constexpr int kLogoId = 0x5C0D'0006;      // Astloch Bold
constexpr int kTitleId = 0x5C0D'0007;     // Astloch Bold, the death screen's headline

void ensure(GfxRenderer& renderer);

}  // namespace scoundrelfonts
