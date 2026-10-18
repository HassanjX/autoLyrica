#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "lyrics_view.h"
#include "theme.h"
#include "imgui.h"

#include <algorithm>
#include <sstream>
#include <cstring>
#include <cmath>

LyricsView::LyricsView() {
}

LyricsView::~LyricsView() {
}

void LyricsView::sync_words_to_text(SegmentItem& seg) {
    std::istringstream iss(seg.text);
    std::vector<std::string> raw_tokens;
    std::string token;
    while (iss >> token) {
        raw_tokens.push_back(token);
    }

    if (raw_tokens.empty()) {
        seg.words.clear();
        return;
    }

    // If word count matches existing words, preserve all exact DTW timings and just update word strings
    if (raw_tokens.size() == seg.words.size()) {
        for (size_t i = 0; i < raw_tokens.size(); ++i) {
            seg.words[i].text = raw_tokens[i];
        }
        return;
    }

    // If word count changed, redistribute words evenly across the segment duration
    int64_t seg_dur = seg.end_ms - seg.start_ms;
    if (seg_dur <= 0) seg_dur = static_cast<int64_t>(raw_tokens.size() * 300);

    int64_t word_dur = seg_dur / raw_tokens.size();
    std::vector<WordItem> new_words;
    for (size_t i = 0; i < raw_tokens.size(); ++i) {
        WordItem wi;
        wi.text = raw_tokens[i];
        wi.start_ms = seg.start_ms + i * word_dur;
        wi.end_ms = wi.start_ms + word_dur;
        wi.confidence = 1.0f;
        new_words.push_back(wi);
    }
    seg.words = std::move(new_words);
}

void LyricsView::shift_all_timestamps(std::vector<SegmentItem>& segments, int64_t offset_ms) {
    for (auto& seg : segments) {
        seg.start_ms = std::max<int64_t>(0, seg.start_ms + offset_ms);
        seg.end_ms = std::max<int64_t>(seg.start_ms, seg.end_ms + offset_ms);
        for (auto& w : seg.words) {
            w.start_ms = std::max<int64_t>(0, w.start_ms + offset_ms);
            w.end_ms = std::max<int64_t>(w.start_ms, w.end_ms + offset_ms);
        }
    }
}

void LyricsView::render(std::vector<SegmentItem>& segments, AudioPlayer& player, int64_t current_time_ms) {
    if (segments.empty()) {
        ImGui::BeginChild("LyricsEmptyView", ImVec2(0, 0), true);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);
        
        const char* msg = "No transcript yet. Load an audio or music file and click 'START TRANSCRIBE'.";
        float text_width = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - text_width) * 0.5f);
        ImGui::TextColored(Theme::TextMuted, "%s", msg);

        const char* hint = "Drop any MP3, WAV, FLAC, M4A, or MP4 file onto the window.";
        float hint_width = ImGui::CalcTextSize(hint).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - hint_width) * 0.5f);
        ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "%s", hint);

        ImGui::EndChild();
        return;
    }

    // Top Controls Bar for Lyrics
    ImGui::BeginChild("LyricsControlsBar", ImVec2(0, 42), false);
    ImGui::Checkbox("Auto-Scroll", &m_auto_scroll);
    ImGui::SameLine();

    // Prominent Edit Mode toggle button
    if (m_edit_mode) {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::HighlightGold);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08f, 0.09f, 0.12f, 1.0f));
        if (ImGui::Button(" [Editing Mode Active] ", ImVec2(160, 28))) {
            m_edit_mode = false;
        }
        ImGui::PopStyleColor(2);
    } else {
        if (ImGui::Button("Edit Lyrics & Timing", ImVec2(150, 28))) {
            m_edit_mode = true;
        }
    }

    ImGui::SameLine(0, 16);
    ImGui::TextColored(Theme::TextMuted, "| %zu lines", segments.size());

    if (m_edit_mode) {
        // Global timestamp shift controls
        ImGui::SameLine(ImGui::GetWindowWidth() - 380);
        ImGui::TextColored(Theme::AccentCyan, "Shift All:");
        ImGui::SameLine();
        if (ImGui::Button("-0.5s##ShiftAll")) shift_all_timestamps(segments, -500);
        ImGui::SameLine();
        if (ImGui::Button("-0.1s##ShiftAll")) shift_all_timestamps(segments, -100);
        ImGui::SameLine();
        if (ImGui::Button("+0.1s##ShiftAll")) shift_all_timestamps(segments, 100);
        ImGui::SameLine();
        if (ImGui::Button("+0.5s##ShiftAll")) shift_all_timestamps(segments, 500);
    }

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Spacing();

    // Main Scrollable Lyrics Container
    ImGui::BeginChild("LyricsScrollArea", ImVec2(0, 0), true);

    int active_seg_id = -1;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (current_time_ms >= segments[i].start_ms && current_time_ms <= segments[i].end_ms) {
            active_seg_id = static_cast<int>(i);
            break;
        }
    }

    int delete_idx = -1;
    int insert_idx = -1;
    int merge_idx = -1;

    for (size_t i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];
        bool is_line_active = (static_cast<int>(i) == active_seg_id);

        ImGui::PushID(static_cast<int>(i));

        // Auto-scroll target
        if (is_line_active && m_auto_scroll && !m_edit_mode && (m_last_active_segment_id != active_seg_id)) {
            ImGui::SetScrollHereY(0.4f);
            m_last_active_segment_id = active_seg_id;
        }

        // Card styling
        if (is_line_active) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.16f, 0.32f, 0.65f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.15f, 0.40f));
        }

        float card_height = m_edit_mode ? 112.0f : 54.0f;
        ImGui::BeginChild("LineCard", ImVec2(0, card_height), true, ImGuiWindowFlags_NoScrollbar);

        // Time Badge Button (Clicking jumps to start of line)
        std::string ts_label = WhisperEngine::format_timestamp(seg.start_ms, false);
        ImGui::PushStyleColor(ImGuiCol_Button, is_line_active ? Theme::AccentPurple : ImVec4(0.16f, 0.18f, 0.25f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Text, is_line_active ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : Theme::TextMuted);
        if (ImGui::Button(ts_label.c_str(), ImVec2(60, 26))) {
            player.seek_ms(seg.start_ms);
        }
        ImGui::PopStyleColor(2);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Jump to %s", WhisperEngine::format_timestamp(seg.start_ms).c_str());
        }

        ImGui::SameLine();

        if (!m_edit_mode) {
            // ==========================================
            // KARAOKE PLAYBACK MODE (Word-by-word Highlight)
            // ==========================================
            if (!seg.words.empty()) {
                for (size_t w = 0; w < seg.words.size(); ++w) {
                    auto& word = seg.words[w];
                    bool is_word_active = (current_time_ms >= word.start_ms && current_time_ms <= word.end_ms);
                    bool has_sung = (current_time_ms > word.end_ms);

                    ImGui::PushID(static_cast<int>(w));

                    if (is_word_active) {
                        ImGui::PushStyleColor(ImGuiCol_Text, Theme::HighlightGold);
                    } else if (has_sung && is_line_active) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                    } else if (is_line_active) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.80f, 1.0f));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.86f, 0.90f, 0.85f));
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.42f, 0.36f, 0.91f, 0.30f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.36f, 0.91f, 0.60f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

                    if (ImGui::Button(word.text.c_str())) {
                        player.seek_ms(word.start_ms);
                    }

                    // Context Menu for Word
                    if (ImGui::BeginPopupContextItem()) {
                        ImGui::Text("Word: \"%s\"", word.text.c_str());
                        ImGui::Separator();
                        if (ImGui::MenuItem("Edit this word...")) {
                            m_edit_seg_idx = static_cast<int>(i);
                            m_edit_word_idx = static_cast<int>(w);
                            strncpy_s(m_word_edit_buf, word.text.c_str(), sizeof(m_word_edit_buf));
                            m_word_edit_start = word.start_ms;
                            m_word_edit_end = word.end_ms;
                            m_open_word_modal = true;
                        }
                        if (ImGui::MenuItem("Play from this word")) {
                            player.seek_ms(word.start_ms);
                            player.play();
                        }
                        if (ImGui::MenuItem("Set word start to current playhead")) {
                            word.start_ms = player.get_position_ms();
                            if (word.end_ms <= word.start_ms) word.end_ms = word.start_ms + 250;
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Word: \"%s\"\nTime: %s -> %s\nConfidence: %.0f%%\n(Left-click seek, Right-click edit)",
                                          word.text.c_str(),
                                          WhisperEngine::format_timestamp(word.start_ms).c_str(),
                                          WhisperEngine::format_timestamp(word.end_ms).c_str(),
                                          word.confidence * 100.0f);
                    }

                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(4);
                    ImGui::PopID();

                    if (w + 1 < seg.words.size()) {
                        ImGui::SameLine(0, 4);
                    }
                }
            } else {
                ImGui::Text("%s", seg.text.c_str());
            }
        } else {
            // ==========================================
            // COMPREHENSIVE LYRICS STUDIO EDITOR MODE
            // ==========================================
            
            // Row 1: Line Text Input + Quick Action Buttons
            char buf[512];
            strncpy_s(buf, seg.text.c_str(), sizeof(buf));
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 420);
            if (ImGui::InputText("##EditLineText", buf, sizeof(buf))) {
                seg.text = buf;
                sync_words_to_text(seg);
            }

            // "Set to Playhead" Button
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentPurple);
            if (ImGui::Button(" 🎯 Set to Now ", ImVec2(100, 24))) {
                int64_t cur = player.get_position_ms();
                int64_t dur = seg.end_ms - seg.start_ms;
                seg.start_ms = cur;
                seg.end_ms = cur + ((dur > 0) ? dur : 2500);
                for (auto& w : seg.words) {
                    w.start_ms = cur;
                    w.end_ms = cur + 300;
                }
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Align start of line to current music playhead (%s)", WhisperEngine::format_timestamp(player.get_position_ms()).c_str());

            // Nudge Timings Buttons
            ImGui::SameLine();
            if (ImGui::Button("-0.5s")) {
                seg.start_ms = std::max<int64_t>(0, seg.start_ms - 500);
                seg.end_ms = std::max<int64_t>(seg.start_ms, seg.end_ms - 500);
                for (auto& w : seg.words) {
                    w.start_ms = std::max<int64_t>(0, w.start_ms - 500);
                    w.end_ms = std::max<int64_t>(w.start_ms, w.end_ms - 500);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("-0.1s")) {
                seg.start_ms = std::max<int64_t>(0, seg.start_ms - 100);
                seg.end_ms = std::max<int64_t>(seg.start_ms, seg.end_ms - 100);
                for (auto& w : seg.words) {
                    w.start_ms = std::max<int64_t>(0, w.start_ms - 100);
                    w.end_ms = std::max<int64_t>(w.start_ms, w.end_ms - 100);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("+0.1s")) {
                seg.start_ms += 100;
                seg.end_ms += 100;
                for (auto& w : seg.words) {
                    w.start_ms += 100;
                    w.end_ms += 100;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("+0.5s")) {
                seg.start_ms += 500;
                seg.end_ms += 500;
                for (auto& w : seg.words) {
                    w.start_ms += 500;
                    w.end_ms += 500;
                }
            }

            // Line Action Buttons (Add Below, Merge, Delete)
            ImGui::SameLine();
            if (ImGui::Button("+ Below")) {
                insert_idx = static_cast<int>(i);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Insert an empty lyric line below this one");

            if (i + 1 < segments.size()) {
                ImGui::SameLine();
                if (ImGui::Button("Merge ↓")) {
                    merge_idx = static_cast<int>(i);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Merge this line with the line below");
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.82f, 0.22f, 0.22f, 0.8f));
            if (ImGui::Button(" ✕ ")) {
                delete_idx = static_cast<int>(i);
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete this lyric line");

            // Row 2: Word Chips (Click any word chip to edit word text & timing directly)
            ImGui::Spacing();
            ImGui::SetCursorPosX(72);
            ImGui::TextColored(Theme::AccentCyan, "Word Timings (click to edit):");
            ImGui::SameLine();

            for (size_t w = 0; w < seg.words.size(); ++w) {
                auto& word = seg.words[w];
                ImGui::PushID(static_cast<int>(w));

                std::string word_btn = word.text + " [" + WhisperEngine::format_timestamp(word.start_ms, false) + "]";
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.23f, 0.33f, 0.8f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));

                if (ImGui::Button(word_btn.c_str())) {
                    m_edit_seg_idx = static_cast<int>(i);
                    m_edit_word_idx = static_cast<int>(w);
                    strncpy_s(m_word_edit_buf, word.text.c_str(), sizeof(m_word_edit_buf));
                    m_word_edit_start = word.start_ms;
                    m_word_edit_end = word.end_ms;
                    m_open_word_modal = true;
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click to edit \"%s\" (Start: %s, End: %s)",
                                      word.text.c_str(),
                                      WhisperEngine::format_timestamp(word.start_ms).c_str(),
                                      WhisperEngine::format_timestamp(word.end_ms).c_str());
                }

                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::PopID();

                if (w + 1 < seg.words.size()) {
                    ImGui::SameLine(0, 4);
                }
            }
        }

        ImGui::EndChild(); // LineCard
        ImGui::PopStyleColor(); // ChildBg
        ImGui::PopID();
        ImGui::Spacing();
    }

    // Handle Line Management Operations
    if (delete_idx >= 0 && delete_idx < static_cast<int>(segments.size())) {
        segments.erase(segments.begin() + delete_idx);
    }
    if (insert_idx >= 0 && insert_idx < static_cast<int>(segments.size())) {
        SegmentItem new_seg;
        new_seg.id = static_cast<int>(segments.size());
        new_seg.start_ms = segments[insert_idx].end_ms;
        new_seg.end_ms = new_seg.start_ms + 2500;
        new_seg.text = "New lyrics line";
        sync_words_to_text(new_seg);
        segments.insert(segments.begin() + insert_idx + 1, new_seg);
    }
    if (merge_idx >= 0 && merge_idx + 1 < static_cast<int>(segments.size())) {
        auto& first = segments[merge_idx];
        const auto& second = segments[merge_idx + 1];
        first.text += " " + second.text;
        first.end_ms = second.end_ms;
        for (const auto& w : second.words) {
            first.words.push_back(w);
        }
        segments.erase(segments.begin() + merge_idx + 1);
    }

    // Render Word Editor Modal if active
    render_word_editor_modal(segments, player);

    ImGui::EndChild(); // LyricsScrollArea
}

void LyricsView::render_word_editor_modal(std::vector<SegmentItem>& segments, AudioPlayer& player) {
    if (m_open_word_modal) {
        ImGui::OpenPopup("WordEditorModal");
        m_open_word_modal = false;
    }

    if (ImGui::BeginPopupModal("WordEditorModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_edit_seg_idx >= 0 && m_edit_seg_idx < static_cast<int>(segments.size()) &&
            m_edit_word_idx >= 0 && m_edit_word_idx < static_cast<int>(segments[m_edit_seg_idx].words.size())) {

            auto& word = segments[m_edit_seg_idx].words[m_edit_word_idx];

            ImGui::TextColored(Theme::AccentCyan, "Edit Word & Timestamp Alignment");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Word Text:");
            ImGui::InputText("##ModalWordText", m_word_edit_buf, sizeof(m_word_edit_buf));

            ImGui::Spacing();
            ImGui::Text("Start Time: %s (%lld ms)", WhisperEngine::format_timestamp(m_word_edit_start).c_str(), m_word_edit_start);
            ImGui::SameLine();
            if (ImGui::Button(" 🎯 Set to Now##WordStart ")) {
                m_word_edit_start = player.get_position_ms();
                if (m_word_edit_end <= m_word_edit_start) m_word_edit_end = m_word_edit_start + 250;
            }

            if (ImGui::Button("-100ms##WS")) m_word_edit_start = std::max<int64_t>(0, m_word_edit_start - 100);
            ImGui::SameLine();
            if (ImGui::Button("+100ms##WS")) m_word_edit_start += 100;
            ImGui::SameLine();
            if (ImGui::Button("-500ms##WS")) m_word_edit_start = std::max<int64_t>(0, m_word_edit_start - 500);
            ImGui::SameLine();
            if (ImGui::Button("+500ms##WS")) m_word_edit_start += 500;

            ImGui::Spacing();
            ImGui::Text("End Time:   %s (%lld ms)", WhisperEngine::format_timestamp(m_word_edit_end).c_str(), m_word_edit_end);
            if (ImGui::Button("-100ms##WE")) m_word_edit_end = std::max<int64_t>(m_word_edit_start, m_word_edit_end - 100);
            ImGui::SameLine();
            if (ImGui::Button("+100ms##WE")) m_word_edit_end += 100;
            ImGui::SameLine();
            if (ImGui::Button("-500ms##WE")) m_word_edit_end = std::max<int64_t>(m_word_edit_start, m_word_edit_end - 500);
            ImGui::SameLine();
            if (ImGui::Button("+500ms##WE")) m_word_edit_end += 500;

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Save Changes", ImVec2(120, 32))) {
                word.text = m_word_edit_buf;
                word.start_ms = m_word_edit_start;
                word.end_ms = m_word_edit_end;

                // Reconstruct segment text
                std::string full_txt;
                for (size_t wi = 0; wi < segments[m_edit_seg_idx].words.size(); ++wi) {
                    full_txt += segments[m_edit_seg_idx].words[wi].text;
                    if (wi + 1 < segments[m_edit_seg_idx].words.size()) full_txt += " ";
                }
                segments[m_edit_seg_idx].text = full_txt;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Play Audio Here", ImVec2(120, 32))) {
                player.seek_ms(m_word_edit_start);
                player.play();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 32))) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
