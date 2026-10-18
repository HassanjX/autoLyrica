#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "app_ui.h"
#include "theme.h"
#include "imgui.h"

#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

AppUI::AppUI() : m_models("models") {
    init();
}

AppUI::~AppUI() {
    m_player.stop();
    m_whisper.cancel_transcription();
}

void AppUI::init() {
    Theme::apply_modern_dark_theme();
    m_models.refresh_local_models();
    
    // Select default model
    const auto& avail = m_models.get_available_models();
    if (!avail.empty()) {
        m_selected_model_name = avail[0].name;
        // If an already downloaded model exists, pick it
        for (const auto& m : avail) {
            if (m.is_downloaded) {
                m_selected_model_name = m.name;
                break;
            }
        }
    }
    m_status_message = "Ready. Open an audio file to begin.";
}

void AppUI::handle_dropped_file(const std::string& filepath) {
    m_current_filepath = filepath;
    m_status_message = "Loading audio file: " + filepath;

    std::string err;
    if (!AudioDecoder::decode_to_whisper_pcm(filepath, m_audio_data, &err)) {
        m_status_message = "Error decoding audio: " + err;
        return;
    }

    if (!m_player.load_file(filepath)) {
        m_status_message = "Decoded for transcription, but failed to load audio playback.";
    } else {
        m_status_message = "Audio loaded successfully (" + WhisperEngine::format_timestamp(m_audio_data.duration_ms, false) + ")";
    }

    m_transcription.segments.clear();
}

void AppUI::open_file_dialog() {
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"All Audio & Music Files (*.mp3;*.wav;*.flac;*.m4a;*.ogg;*.aac;*.mp4;*.wma)\0*.mp3;*.wav;*.flac;*.m4a;*.ogg;*.aac;*.mp4;*.wma\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        int len = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            std::string path(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &path[0], len, NULL, NULL);
            handle_dropped_file(path);
        }
    }
}

void AppUI::render() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("autoLyricaMainWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Global keyboard shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !ImGui::GetIO().WantTextInput) {
        m_player.toggle_play();
    }

    render_top_bar();

    // Main workspace split: Left Sidebar (Controls) + Center/Right (Lyrics View)
    float sidebar_width = 320.0f;
    float bottom_player_height = 115.0f;
    float content_height = ImGui::GetContentRegionAvail().y - bottom_player_height;

    // Sidebar
    ImGui::SetCursorPos(ImVec2(12, 54));
    ImGui::BeginChild("SidebarPanel", ImVec2(sidebar_width, content_height), true);
    render_sidebar();
    ImGui::EndChild();

    // Center Lyrics View
    ImGui::SetCursorPos(ImVec2(sidebar_width + 24, 54));
    float lyrics_width = ImGui::GetWindowWidth() - sidebar_width - 36;
    ImGui::BeginChild("MainLyricsPanel", ImVec2(lyrics_width, content_height), true);
    int64_t current_pos_ms = m_player.get_position_ms();
    m_lyrics_view.render(m_transcription.segments, m_player, current_pos_ms);
    ImGui::EndChild();

    // Bottom Audio Player Bar
    ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowHeight() - bottom_player_height));
    ImGui::BeginChild("BottomPlayerBar", ImVec2(ImGui::GetWindowWidth(), bottom_player_height), true, ImGuiWindowFlags_NoScrollbar);
    render_bottom_player();
    ImGui::EndChild();

    ImGui::End();
}

void AppUI::render_top_bar() {
    if (ImGui::BeginMenuBar()) {
        // App Logo
        ImGui::TextColored(Theme::AccentPurple, " [ autoLyrica ] ");
        ImGui::SameLine();
        ImGui::TextColored(Theme::AccentCyan, "Word-by-Word AI Lyrics Transcriber");

        ImGui::SameLine(ImGui::GetWindowWidth() - 320);
        ImGui::TextColored(Theme::SuccessGreen, "● High-Perf CPU (AVX2+FMA)");

        ImGui::EndMenuBar();
    }
}

void AppUI::render_sidebar() {
    ImGui::TextColored(Theme::AccentCyan, "AUDIO INPUT");
    ImGui::Spacing();

    if (ImGui::Button("Open Audio File...", ImVec2(-1, 34))) {
        open_file_dialog();
    }

    if (!m_current_filepath.empty()) {
        std::string filename = m_current_filepath.substr(m_current_filepath.find_last_of("/\\") + 1);
        ImGui::TextWrapped("File: %s", filename.c_str());
        ImGui::TextColored(Theme::TextMuted, "Duration: %s", WhisperEngine::format_timestamp(m_audio_data.duration_ms, false).c_str());
    } else {
        ImGui::TextColored(Theme::TextMuted, "Drag & drop audio here");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Whisper Model Section
    ImGui::TextColored(Theme::AccentCyan, "WHISPER MODEL");
    ImGui::Spacing();

    const auto& models = m_models.get_available_models();
    if (ImGui::BeginCombo("Model", m_selected_model_name.c_str())) {
        for (const auto& m : models) {
            bool is_selected = (m_selected_model_name == m.name);
            std::string label = m.name + (m.is_downloaded ? " [Ready]" : " [" + m.size_desc + "]");
            if (ImGui::Selectable(label.c_str(), is_selected)) {
                m_selected_model_name = m.name;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Find selected model status
    const ModelInfo* selected_info = nullptr;
    for (const auto& m : models) {
        if (m.name == m_selected_model_name) {
            selected_info = &m;
            break;
        }
    }

    if (selected_info) {
        if (selected_info->is_downloaded) {
            ImGui::TextColored(Theme::SuccessGreen, "Model is downloaded and ready.");
        } else {
            ImGui::TextColored(Theme::HighlightGold, "Model not downloaded (%s)", selected_info->size_desc.c_str());

            if (m_models.is_downloading()) {
                ImGui::ProgressBar(m_models.get_download_progress(), ImVec2(-1, 24));
                ImGui::TextWrapped("%s", m_models.get_download_status().c_str());
                if (ImGui::Button("Cancel Download", ImVec2(-1, 26))) {
                    m_models.cancel_download();
                }
            } else {
                if (ImGui::Button("Download Selected Model", ImVec2(-1, 32))) {
                    m_models.start_download(m_selected_model_name, nullptr, [this](bool success, const std::string& err) {
                        m_status_message = success ? "Model downloaded successfully!" : ("Download error: " + err);
                    });
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Transcription Settings
    ImGui::TextColored(Theme::AccentCyan, "TRANSCRIBE OPTIONS");
    ImGui::Spacing();

    const char* languages[] = { "auto (Detect)", "en (English)", "es (Spanish)", "fr (French)", "de (German)", "ja (Japanese)", "ko (Korean)", "zh (Chinese)", "ar (Arabic)" };
    const char* lang_codes[] = { "auto", "en", "es", "fr", "de", "ja", "ko", "zh", "ar" };
    static int current_lang_idx = 0;

    if (ImGui::Combo("Language", &current_lang_idx, languages, IM_ARRAYSIZE(languages))) {
        m_selected_language = lang_codes[current_lang_idx];
    }

    ImGui::TextColored(Theme::SuccessGreen, "Acceleration: AVX2 + FMA Vectorized");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start Transcribe Button
    bool can_transcribe = m_audio_data.is_loaded && selected_info && selected_info->is_downloaded && !m_whisper.is_transcribing();

    if (m_whisper.is_transcribing()) {
        ImGui::ProgressBar(m_whisper.get_current_progress(), ImVec2(-1, 30));
        ImGui::TextColored(Theme::HighlightGold, "Transcribing with DTW word alignment...");
        if (ImGui::Button("Cancel Transcription", ImVec2(-1, 32))) {
            m_whisper.cancel_transcription();
        }
    } else {
        if (!can_transcribe) {
            ImGui::BeginDisabled();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, Theme::AccentPurple);
        if (ImGui::Button("START TRANSCRIBE", ImVec2(-1, 40))) {
            WhisperConfig cfg;
            cfg.model_path = selected_info->local_path;
            cfg.language = m_selected_language;
            cfg.use_gpu = m_use_gpu;
            cfg.token_timestamps = true;
            cfg.dtw = true;
            cfg.num_threads = 4;

            m_status_message = "Transcribing audio word-by-word...";
            m_whisper.transcribe_async(m_audio_data.pcm_data, cfg,
                [this](float /*progress*/, const std::string& /*current_text*/) {},
                [this](const TranscriptionResult& result) {
                    m_transcription = result;
                    if (result.success) {
                        m_status_message = "Transcription complete! Processed in " + 
                                           std::to_string(result.processing_time_ms / 1000.0f) + "s";
                    } else {
                        m_status_message = "Transcription error: " + result.error_message;
                    }
                }
            );
        }
        ImGui::PopStyleColor();

        if (!can_transcribe) {
            ImGui::EndDisabled();
            if (!m_audio_data.is_loaded) {
                ImGui::TextColored(Theme::TextMuted, "Load an audio file first.");
            } else if (!selected_info || !selected_info->is_downloaded) {
                ImGui::TextColored(Theme::TextMuted, "Download the model first.");
            }
        }
    }

    ImGui::Spacing();
    ImGui::TextColored(Theme::TextMuted, "%s", m_status_message.c_str());
}

void AppUI::render_bottom_player() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 6));

    int64_t current_ms = m_player.get_position_ms();
    int64_t total_ms = m_player.get_duration_ms();
    if (total_ms == 0 && m_audio_data.is_loaded) total_ms = m_audio_data.duration_ms;

    // Row 1: Interactive Waveform Seeker & Timeline
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float timeline_height = 36.0f;

    // Draw waveform peaks if loaded
    if (!m_audio_data.waveform_peaks.empty()) {
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImVec2(avail.x - 280, timeline_height);
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(22, 24, 34, 255), 4.0f);

        float progress_ratio = (total_ms > 0) ? (static_cast<float>(current_ms) / total_ms) : 0.0f;
        float progress_x = canvas_p0.x + progress_ratio * canvas_sz.x;

        // Waveform bars
        const auto& peaks = m_audio_data.waveform_peaks;
        float bar_width = canvas_sz.x / peaks.size();
        for (size_t b = 0; b < peaks.size(); ++b) {
            float bx = canvas_p0.x + b * bar_width;
            float bh = peaks[b] * (timeline_height - 6.0f);
            float by0 = canvas_p0.y + (timeline_height - bh) * 0.5f;
            float by1 = by0 + bh;

            float bar_w = (bar_width > 2.0f) ? (bar_width - 1.0f) : 1.0f;
            ImU32 bar_col = (bx <= progress_x) ? IM_COL32(108, 92, 231, 230) : IM_COL32(70, 75, 95, 180);
            draw_list->AddRectFilled(ImVec2(bx, by0), ImVec2(bx + bar_w, by1), bar_col);
        }

        // Playhead cursor line
        draw_list->AddLine(ImVec2(progress_x, canvas_p0.y), ImVec2(progress_x, canvas_p1.y), IM_COL32(0, 206, 201, 255), 2.0f);

        // Invisible button over waveform for seeking
        ImGui::InvisibleButton("WaveformSeeker", canvas_sz);
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float mouse_x = ImGui::GetIO().MousePos.x - canvas_p0.x;
            float ratio = mouse_x / canvas_sz.x;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            m_player.seek_ms(static_cast<int64_t>(ratio * total_ms));
        }

        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::Text("%s / %s", WhisperEngine::format_timestamp(current_ms, false).c_str(),
                               WhisperEngine::format_timestamp(total_ms, false).c_str());
    } else {
        // Standard Slider if no waveform data yet
        float pos_sec = current_ms / 1000.0f;
        float total_sec = total_ms / 1000.0f;
        float max_time = (total_sec > 1.0f) ? total_sec : 1.0f;
        ImGui::SetNextItemWidth(avail.x - 280);
        if (ImGui::SliderFloat("##TimeSeeker", &pos_sec, 0.0f, max_time, "")) {
            m_player.seek_ms(static_cast<int64_t>(pos_sec * 1000));
        }
        ImGui::SameLine();
        ImGui::Text("%s / %s", WhisperEngine::format_timestamp(current_ms, false).c_str(),
                               WhisperEngine::format_timestamp(total_ms, false).c_str());
    }


    ImGui::Spacing();

    // Row 2: Playback buttons & Export menu
    if (m_player.is_playing()) {
        if (ImGui::Button(" PAUSE [Space] ", ImVec2(120, 32))) {
            m_player.pause();
        }
    } else {
        if (ImGui::Button(" PLAY [Space] ", ImVec2(120, 32))) {
            m_player.play();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(" STOP ", ImVec2(70, 32))) {
        m_player.stop();
    }

    ImGui::SameLine(0, 24);
    ImGui::Text("Volume");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##VolSlider", &m_volume, 0.0f, 1.0f, "%.0f%%")) {
        m_player.set_volume(m_volume);
    }

    // Export Options
    ImGui::SameLine(ImGui::GetWindowWidth() - 560);
    bool has_lyrics = !m_transcription.segments.empty();
    if (!has_lyrics) ImGui::BeginDisabled();

    if (ImGui::Button("Export Enhanced LRC (Karaoke)", ImVec2(200, 32))) {
        std::string save_path = LyricExporter::open_save_dialog(ExportFormat::EnhancedLRC, "lyrics");
        if (!save_path.empty()) {
            LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::EnhancedLRC);
            m_status_message = "Exported Enhanced LRC: " + save_path;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export Standard LRC", ImVec2(150, 32))) {
        std::string save_path = LyricExporter::open_save_dialog(ExportFormat::StandardLRC, "lyrics");
        if (!save_path.empty()) {
            LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::StandardLRC);
            m_status_message = "Exported Standard LRC: " + save_path;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("SRT / JSON / TXT...", ImVec2(150, 32))) {
        ImGui::OpenPopup("ExportMorePopup");
    }

    if (ImGui::BeginPopup("ExportMorePopup")) {
        if (ImGui::MenuItem("SubRip Subtitle (.srt)")) {
            std::string save_path = LyricExporter::open_save_dialog(ExportFormat::SRT, "lyrics");
            if (!save_path.empty()) LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::SRT);
        }
        if (ImGui::MenuItem("WebVTT Subtitle (.vtt)")) {
            std::string save_path = LyricExporter::open_save_dialog(ExportFormat::VTT, "lyrics");
            if (!save_path.empty()) LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::VTT);
        }
        if (ImGui::MenuItem("Detailed JSON Metadata (.json)")) {
            std::string save_path = LyricExporter::open_save_dialog(ExportFormat::JSON, "lyrics");
            if (!save_path.empty()) LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::JSON);
        }
        if (ImGui::MenuItem("Plain Text (.txt)")) {
            std::string save_path = LyricExporter::open_save_dialog(ExportFormat::TXT, "lyrics");
            if (!save_path.empty()) LyricExporter::export_to_file(save_path, m_transcription.segments, ExportFormat::TXT);
        }
        ImGui::EndPopup();
    }

    if (!has_lyrics) ImGui::EndDisabled();

    ImGui::PopStyleVar();
}
