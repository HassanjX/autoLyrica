#pragma once

#include "imgui.h"

class Theme {
public:
    static void apply_modern_dark_theme();

    // Color definitions for UI elements
    static const ImVec4 AccentPurple;
    static const ImVec4 AccentCyan;
    static const ImVec4 HighlightGold;
    static const ImVec4 TextMuted;
    static const ImVec4 BgDark;
    static const ImVec4 CardBg;
    static const ImVec4 SuccessGreen;
};
