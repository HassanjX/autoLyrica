#pragma once

#include "audio/audio_decoder.h"
#include "audio/audio_player.h"
#include "transcriber/whisper_engine.h"
#include "transcriber/model_manager.h"
#include "exporter/lyric_exporter.h"
#include "lyrics_view.h"

#include <string>
#include <vector>
#include <memory>

class AppUI {
public:
    AppUI();
    ~AppUI();

    void init();
    void render();
    void handle_dropped_file(const std::string& filepath);

    bool should_exit() const { return m_should_exit; }

private:
    void render_top_bar();
    void render_sidebar();
    void render_bottom_player();
    void open_file_dialog();

    // Core services
    AudioPlayer m_player;
    WhisperEngine m_whisper;
    ModelManager m_models;
    LyricsView m_lyrics_view;

    // State
    AudioData m_audio_data;
    TranscriptionResult m_transcription;
    std::string m_current_filepath;
    std::string m_status_message;
    std::string m_selected_model_name;
    std::string m_selected_language = "auto";
    bool m_use_gpu = false;
    bool m_should_exit = false;
    void trigger_transcribe();

    // Waveform / Timeline
    float m_volume = 0.85f;
    bool m_is_seeking = false;
    float m_seek_target_ratio = 0.0f;
    bool m_auto_transcribe_after_download = false;
};

