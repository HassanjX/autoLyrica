#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

#include "audio/audio_decoder.h"
#include "transcriber/whisper_engine.h"
#include "exporter/lyric_exporter.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " autoLyrica End-to-End Pipeline Verification " << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Audio Decoding Test (MP3 Decoding)
    std::string sample_file = "tests/Claude Pop - I'm Upping My P(Doom).mp3";
    std::cout << "[Step 1] Decoding MP3 sample: " << sample_file << std::endl;

    AudioData audio;
    std::string err;
    bool decode_ok = AudioDecoder::decode_to_whisper_pcm(sample_file, audio, &err);
    if (!decode_ok) {
        std::cerr << "FAIL: Audio decoding failed: " << err << std::endl;
        return 1;
    }

    std::cout << "  Decoded Samples : " << audio.pcm_data.size() << std::endl;
    std::cout << "  Sample Rate     : " << audio.sample_rate << " Hz" << std::endl;
    std::cout << "  Duration        : " << audio.duration_ms << " ms (" 
              << WhisperEngine::format_timestamp(audio.duration_ms) << ")" << std::endl;
    std::cout << "  Waveform Peaks  : " << audio.waveform_peaks.size() << " bins calculated." << std::endl;

    assert(!audio.pcm_data.empty());
    assert(audio.sample_rate == 16000);
    assert(audio.duration_ms > 0);
    assert(!audio.waveform_peaks.empty());
    std::cout << "PASS: Audio decoding succeeded!\n" << std::endl;

    // 2. Whisper Transcription & Word-Level Timestamp Test
    std::string model_file = "models/ggml-base.en.bin";
    std::cout << "[Step 2] Transcribing with model: " << model_file << std::endl;

    WhisperEngine engine;
    WhisperConfig config;
    config.model_path = model_file;
    config.language = "en";
    config.use_gpu = false; // CPU AVX2
    config.token_timestamps = true;
    config.dtw = true;
    config.num_threads = 4;

    bool finished = false;
    TranscriptionResult result;

    bool start_ok = engine.transcribe_async(
        audio.pcm_data,
        config,
        [](float prog, const std::string& text) {
            std::cout << "  [Progress " << (int)(prog * 100) << "%] " << text << std::endl;
        },
        [&](const TranscriptionResult& res) {
            result = res;
            finished = true;
        }
    );

    if (!start_ok) {
        std::cerr << "FAIL: Failed to start transcription." << std::endl;
        return 1;
    }

    // Wait for worker thread to complete
    while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!result.success) {
        std::cerr << "FAIL: Transcription error: " << result.error_message << std::endl;
        return 1;
    }

    std::cout << "\nPASS: Transcription completed in " << result.processing_time_ms << " ms!" << std::endl;
    std::cout << "Detected Language: " << result.detected_language << std::endl;
    std::cout << "Segments count: " << result.segments.size() << std::endl;

    assert(!result.segments.empty());

    // Print segments and word-by-word timestamps
    int total_words = 0;
    for (const auto& seg : result.segments) {
        std::cout << "\n--- Segment [" << WhisperEngine::format_timestamp(seg.start_ms)
                  << " -> " << WhisperEngine::format_timestamp(seg.end_ms) << "] ---" << std::endl;
        std::cout << "Text: \"" << seg.text << "\"" << std::endl;
        std::cout << "Words (" << seg.words.size() << "): ";
        for (const auto& w : seg.words) {
            std::cout << "[" << w.text << " @ " << WhisperEngine::format_timestamp(w.start_ms) << " (" << (int)(w.confidence * 100) << "%)] ";
            total_words++;
        }
        std::cout << std::endl;
    }

    assert(total_words > 0);
    std::cout << "\nTotal words with timestamps: " << total_words << std::endl;

    // 3. Exporter Formats Test
    std::cout << "\n[Step 3] Testing Exporters..." << std::endl;

    std::string enhanced_lrc = LyricExporter::export_to_string(result.segments, ExportFormat::EnhancedLRC, "JFK Speech");
    std::cout << "--- Enhanced Karaoke LRC Output ---\n" << enhanced_lrc << std::endl;
    assert(!enhanced_lrc.empty());
    assert(enhanced_lrc.find("<") != std::string::npos);

    std::string srt = LyricExporter::export_to_string(result.segments, ExportFormat::SRT, "JFK Speech");
    std::cout << "--- SRT Subtitle Output ---\n" << srt << std::endl;
    assert(!srt.empty());

    std::string json = LyricExporter::export_to_string(result.segments, ExportFormat::JSON, "JFK Speech");
    std::cout << "--- JSON Metadata Output (first 300 chars) ---\n" << json.substr(0, std::min<size_t>(300, json.size())) << "...\n" << std::endl;
    assert(!json.empty());

    std::cout << "========================================" << std::endl;
    std::cout << " ALL VERIFICATION TESTS PASSED SUCCESSFULLY! " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
