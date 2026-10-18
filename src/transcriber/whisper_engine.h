#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <functional>

struct WordItem {
    std::string text;
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    float confidence = 1.0f;
};

struct SegmentItem {
    int id = 0;
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    std::string text;
    std::vector<WordItem> words;
};

struct TranscriptionResult {
    std::vector<SegmentItem> segments;
    std::string detected_language = "en";
    int64_t processing_time_ms = 0;
    bool success = false;
    std::string error_message;
};

struct WhisperConfig {
    std::string model_path;
    std::string language = "auto";
    bool translate = false;
    bool use_gpu = true;
    bool token_timestamps = true;
    bool dtw = true;
    int num_threads = 4;
};

struct whisper_context;

class WhisperEngine {
public:
    WhisperEngine();
    ~WhisperEngine();

    bool load_model(const std::string& model_path, bool use_gpu = true);
    void unload_model();
    bool is_model_loaded() const;
    std::string get_loaded_model_path() const;

    // Asynchronous background transcription
    bool transcribe_async(const std::vector<float>& pcm_16k,
                          const WhisperConfig& config,
                          std::function<void(float progress, const std::string& current_text)> progress_cb,
                          std::function<void(const TranscriptionResult& result)> complete_cb);

    void cancel_transcription();
    bool is_transcribing() const { return m_is_transcribing; }
    float get_current_progress() const { return m_current_progress; }

    static std::string format_timestamp(int64_t ms, bool include_ms = true);

private:
    whisper_context* m_ctx = nullptr;
    std::string m_loaded_model_path;
    bool m_model_loaded = false;
    std::atomic<bool> m_is_transcribing{false};
    std::atomic<bool> m_cancel_requested{false};
    std::atomic<float> m_current_progress{0.0f};
};
