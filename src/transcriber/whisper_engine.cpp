#include "whisper_engine.h"
#include "whisper.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

WhisperEngine::WhisperEngine() {
}

WhisperEngine::~WhisperEngine() {
    unload_model();
}

bool WhisperEngine::load_model(const std::string& model_path, bool use_gpu) {
    if (m_model_loaded && m_loaded_model_path == model_path) {
        return true;
    }

    unload_model();

    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = use_gpu;
    cparams.flash_attn = false;
    cparams.dtw_token_timestamps = true;

    if (model_path.find("tiny.en") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_TINY_EN;
    else if (model_path.find("tiny") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_TINY;
    else if (model_path.find("base.en") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_BASE_EN;
    else if (model_path.find("base") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_BASE;
    else if (model_path.find("small.en") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_SMALL_EN;
    else if (model_path.find("small") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_SMALL;
    else if (model_path.find("medium.en") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_MEDIUM_EN;
    else if (model_path.find("medium") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_MEDIUM;
    else if (model_path.find("large-v3-turbo") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_LARGE_V3_TURBO;
    else if (model_path.find("large-v3") != std::string::npos) cparams.dtw_aheads_preset = WHISPER_AHEADS_LARGE_V3;
    else cparams.dtw_aheads_preset = WHISPER_AHEADS_N_TOP_MOST;
    cparams.dtw_n_top = 4;

    m_ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!m_ctx) {
        // Fallback without DTW preset
        cparams.dtw_token_timestamps = false;
        cparams.use_gpu = use_gpu;
        m_ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    }
    if (!m_ctx && use_gpu) {
        // Fallback to CPU without DTW
        cparams.use_gpu = false;
        cparams.dtw_token_timestamps = false;
        m_ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    }

    if (!m_ctx) {
        std::cerr << "Failed to initialize whisper context from model: " << model_path << std::endl;
        return false;
    }

    m_loaded_model_path = model_path;
    m_model_loaded = true;
    return true;
}

void WhisperEngine::unload_model() {
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
    m_model_loaded = false;
    m_loaded_model_path.clear();
}

bool WhisperEngine::is_model_loaded() const {
    return m_model_loaded && (m_ctx != nullptr);
}

std::string WhisperEngine::get_loaded_model_path() const {
    return m_loaded_model_path;
}

void WhisperEngine::cancel_transcription() {
    if (m_is_transcribing) {
        m_cancel_requested = true;
    }
}

std::string WhisperEngine::format_timestamp(int64_t ms, bool include_ms) {
    if (ms < 0) ms = 0;
    int64_t total_sec = ms / 1000;
    int64_t msec = ms % 1000;
    int64_t hours = total_sec / 3600;
    int64_t mins = (total_sec % 3600) / 60;
    int64_t secs = total_sec % 60;

    std::ostringstream ss;
    if (hours > 0) {
        ss << std::setfill('0') << std::setw(2) << hours << ":";
    }
    ss << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs;

    if (include_ms) {
        ss << "." << std::setfill('0') << std::setw(2) << (msec / 10);
    }
    return ss.str();
}

bool WhisperEngine::transcribe_async(const std::vector<float>& pcm_16k,
                                     const WhisperConfig& config,
                                     std::function<void(float progress, const std::string& current_text)> progress_cb,
                                     std::function<void(const TranscriptionResult& result)> complete_cb) {
    if (m_is_transcribing) return false;
    if (pcm_16k.empty()) {
        if (complete_cb) {
            TranscriptionResult res;
            res.success = false;
            res.error_message = "No audio PCM data provided.";
            complete_cb(res);
        }
        return false;
    }

    if (!load_model(config.model_path, config.use_gpu)) {
        if (complete_cb) {
            TranscriptionResult res;
            res.success = false;
            res.error_message = "Failed to load Whisper model: " + config.model_path;
            complete_cb(res);
        }
        return false;
    }

    m_is_transcribing = true;
    m_cancel_requested = false;
    m_current_progress = 0.0f;

    std::thread([this, pcm_16k, config, progress_cb, complete_cb]() {
        auto start_time = std::chrono::steady_clock::now();
        TranscriptionResult result;

        whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.n_threads = config.num_threads > 0 ? config.num_threads : 4;
        params.translate = config.translate;
        params.language = (config.language == "auto" || config.language.empty()) ? nullptr : config.language.c_str();
        params.detect_language = (config.language == "auto" || config.language.empty());

        params.print_progress = false;
        params.print_special = false;
        params.print_realtime = false;
        params.print_timestamps = false;

        // Word-level timestamps & alignment
        params.token_timestamps = config.token_timestamps;
        params.thold_pt = 0.01f;
        params.thold_ptsum = 0.01f;
        params.max_len = 0;
        params.split_on_word = true;

        // Music & Vocal Transcription Tuning
        params.suppress_nst = true;      // Suppress non-speech tokens like [MUSIC PLAYING]
        params.no_speech_thold = 0.95f;  // Prevent skipping vocal sections over loud instrumentals
        params.temperature = 0.0f;
        params.temperature_inc = 0.2f;   // Fallback temperature if confidence is low
        params.initial_prompt = "Song lyrics:";

        params.progress_callback = [](whisper_context* /*ctx*/, whisper_state* /*state*/, int progress, void* user_data) {
            auto* self = static_cast<WhisperEngine*>(user_data);
            if (self) {
                float p = progress / 100.0f;
                self->m_current_progress = p;
            }
        };
        params.progress_callback_user_data = this;

        params.encoder_begin_callback = [](whisper_context* /*ctx*/, whisper_state* /*state*/, void* user_data) -> bool {
            auto* self = static_cast<WhisperEngine*>(user_data);
            return self ? !self->m_cancel_requested : true;
        };

        int ret = whisper_full(m_ctx, params, pcm_16k.data(), static_cast<int>(pcm_16k.size()));

        if (m_cancel_requested) {
            result.success = false;
            result.error_message = "Transcription cancelled by user.";
            m_is_transcribing = false;
            if (complete_cb) complete_cb(result);
            return;
        }

        if (ret != 0) {
            result.success = false;
            result.error_message = "Whisper transcription failed with error code: " + std::to_string(ret);
            m_is_transcribing = false;
            if (complete_cb) complete_cb(result);
            return;
        }

        // Auto-detected language
        if (params.detect_language) {
            int lang_id = whisper_full_lang_id(m_ctx);
            const char* lang_str = whisper_lang_str(lang_id);
            if (lang_str) result.detected_language = lang_str;
        } else {
            result.detected_language = config.language;
        }

        int n_segments = whisper_full_n_segments(m_ctx);
        for (int i = 0; i < n_segments; ++i) {
            SegmentItem seg;
            seg.id = i;
            seg.start_ms = whisper_full_get_segment_t0(m_ctx, i) * 10;
            seg.end_ms = whisper_full_get_segment_t1(m_ctx, i) * 10;
            const char* seg_text = whisper_full_get_segment_text(m_ctx, i);
            if (seg_text) {
                std::string s = seg_text;
                // Trim leading whitespace
                size_t first = s.find_first_not_of(" \t\r\n");
                seg.text = (first != std::string::npos) ? s.substr(first) : s;
            }

            int n_tokens = whisper_full_n_tokens(m_ctx, i);
            WordItem current_word;

            for (int j = 0; j < n_tokens; ++j) {
                const char* t_str = whisper_full_get_token_text(m_ctx, i, j);
                if (!t_str || t_str[0] == '\0') continue;

                // Skip Whisper internal tokens
                if ((t_str[0] == '<' && t_str[strlen(t_str) - 1] == '>') ||
                    (t_str[0] == '[' && t_str[strlen(t_str) - 1] == ']')) {
                    continue;
                }

                std::string piece = t_str;
                bool is_new_word = false;

                // Check for SentencePiece underscore U+2581 "\xe2\x96\x81" (3 bytes)
                if (piece.size() >= 3 && 
                    (unsigned char)piece[0] == 0xE2 && 
                    (unsigned char)piece[1] == 0x96 && 
                    (unsigned char)piece[2] == 0x81) {
                    is_new_word = true;
                    piece = piece.substr(3);
                } else if (piece[0] == ' ') {
                    is_new_word = true;
                    size_t first = piece.find_first_not_of(" \t\r\n");
                    piece = (first != std::string::npos) ? piece.substr(first) : "";
                }

                if (piece.empty()) continue;

                int64_t t0 = whisper_full_get_token_t0(m_ctx, i, j) * 10;
                int64_t t1 = whisper_full_get_token_t1(m_ctx, i, j) * 10;
                float prob = whisper_full_get_token_p(m_ctx, i, j);

                if (is_new_word && !current_word.text.empty()) {
                    seg.words.push_back(current_word);
                    current_word = WordItem{};
                }

                if (current_word.text.empty()) {
                    current_word.text = piece;
                    current_word.start_ms = (t0 >= 0) ? t0 : seg.start_ms;
                    current_word.end_ms = (t1 >= 0) ? t1 : (current_word.start_ms + 250);
                    current_word.confidence = prob;
                } else {
                    current_word.text += piece;
                    if (t1 > current_word.end_ms) {
                        current_word.end_ms = t1;
                    }
                    current_word.confidence = (current_word.confidence + prob) * 0.5f;
                }
            }


            if (!current_word.text.empty()) {
                seg.words.push_back(current_word);
            }

            // Fallback interpolation: If token timestamps were missing or clamped, distribute words evenly across segment
            if (!seg.words.empty()) {
                int64_t seg_dur = seg.end_ms - seg.start_ms;
                if (seg_dur > 0) {
                    int64_t word_dur = seg_dur / seg.words.size();
                    for (size_t w = 0; w < seg.words.size(); ++w) {
                        if (seg.words[w].start_ms == seg.words[w].end_ms || seg.words[w].start_ms < seg.start_ms) {
                            seg.words[w].start_ms = seg.start_ms + w * word_dur;
                            seg.words[w].end_ms = seg.words[w].start_ms + word_dur;
                        }
                    }
                }
            }

            // Filter out non-speech instrumental placeholders
            if (seg.text.empty() || 
                seg.text == "[MUSIC PLAYING]" || 
                seg.text == "[MUSIC]" || 
                seg.text == "(music)" || 
                seg.text == "(Music)" ||
                seg.text == "[APPLAUSE]" || 
                seg.text == "[LAUGHTER]") {
                continue;
            }

            result.segments.push_back(seg);

            if (progress_cb) {
                progress_cb(static_cast<float>(i + 1) / n_segments, seg.text);
            }

        }

        auto end_time = std::chrono::steady_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        result.success = true;
        m_current_progress = 1.0f;
        m_is_transcribing = false;

        if (complete_cb) {
            complete_cb(result);
        }
    }).detach();

    return true;
}
