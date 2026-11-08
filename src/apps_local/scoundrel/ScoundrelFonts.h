#pragma once

// Scoundrel's own faces, the phone version's: Astloch for the logo, Grenze Bold
// for values, Space Grotesk for everything else. Registered with the renderer
// the first time the game opens; see fonts/ for the cuts.

class GfxRenderer;

namespace scoundrelfonts {

constexpr int kSmallId = 0x5C0D'0001;     // Space Grotesk 600, ~11px caps
constexpr int kBodyId = 0x5C0D'0002;      // Space Grotesk 500, ~14px caps
constexpr int kLargeId = 0x5C0D'0003;     // Space Grotesk 500, ~17px caps
constexpr int kValueId = 0x5C0D'0004;     // Grenze Bold, ~28px caps
constexpr int kValueBigId = 0x5C0D'0005;  // Grenze Bold, ~41px caps
constexpr int kLogoId = 0x5C0D'0006;      // Astloch Bold

void ensure(GfxRenderer& renderer);

}  // namespace scoundrelfonts
