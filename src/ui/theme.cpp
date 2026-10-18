#include "theme.h"

const ImVec4 Theme::AccentPurple = ImVec4(0.42f, 0.36f, 0.91f, 1.0f); // #6C5CE7
const ImVec4 Theme::AccentCyan   = ImVec4(0.00f, 0.81f, 0.79f, 1.0f); // #00CEC9
const ImVec4 Theme::HighlightGold= ImVec4(0.99f, 0.80f, 0.43f, 1.0f); // #FDCB6E
const ImVec4 Theme::TextMuted    = ImVec4(0.60f, 0.63f, 0.72f, 1.0f);
const ImVec4 Theme::BgDark       = ImVec4(0.06f, 0.07f, 0.09f, 1.0f); // #0F1117
const ImVec4 Theme::CardBg       = ImVec4(0.11f, 0.12f, 0.17f, 1.0f); // #1C1F2B
const ImVec4 Theme::SuccessGreen = ImVec4(0.18f, 0.80f, 0.44f, 1.0f); // #2ECC71

void Theme::apply_modern_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Window & Panels
    colors[ImGuiCol_WindowBg]           = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.10f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.09f, 0.10f, 0.14f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.18f, 0.20f, 0.28f, 0.80f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Headers & Titles
    colors[ImGuiCol_FrameBg]            = ImVec4(0.13f, 0.15f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.19f, 0.22f, 0.31f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.24f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.06f, 0.07f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.06f, 0.07f, 0.10f, 0.75f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);

    // Text
    colors[ImGuiCol_Text]               = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.53f, 0.60f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button]             = ImVec4(0.42f, 0.36f, 0.91f, 0.85f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.49f, 0.43f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.35f, 0.29f, 0.82f, 1.00f);

    // Tabs & Headers
    colors[ImGuiCol_Header]             = ImVec4(0.22f, 0.25f, 0.35f, 0.70f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.30f, 0.34f, 0.48f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.42f, 0.36f, 0.91f, 0.90f);
    colors[ImGuiCol_Tab]                = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.42f, 0.36f, 0.91f, 0.80f);
    colors[ImGuiCol_TabSelected]        = ImVec4(0.42f, 0.36f, 0.91f, 1.00f);
    colors[ImGuiCol_TabDimmed]          = ImVec4(0.10f, 0.11f, 0.16f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]  = ImVec4(0.16f, 0.18f, 0.25f, 1.00f);

    // Scrollbars & Sliders
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.06f, 0.07f, 0.09f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.25f, 0.28f, 0.38f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.35f, 0.39f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]= ImVec4(0.42f, 0.36f, 0.91f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.00f, 0.81f, 0.79f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.00f, 0.81f, 0.79f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.00f, 0.81f, 0.79f, 1.00f);

    // Separators & Resize Grips
    colors[ImGuiCol_Separator]          = ImVec4(0.18f, 0.20f, 0.28f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.42f, 0.36f, 0.91f, 0.80f);
    colors[ImGuiCol_SeparatorActive]    = ImVec4(0.42f, 0.36f, 0.91f, 1.00f);
    colors[ImGuiCol_ResizeGrip]         = ImVec4(0.20f, 0.23f, 0.32f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.42f, 0.36f, 0.91f, 0.80f);
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.42f, 0.36f, 0.91f, 1.00f);

    // Plot & Progress
    colors[ImGuiCol_PlotLines]          = ImVec4(0.42f, 0.36f, 0.91f, 1.00f);
    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.00f, 0.81f, 0.79f, 1.00f);

    // Styling Geometry
    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(12.0f, 7.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 12.0f;

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 5.0f;
    style.TabRounding       = 6.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;
}
