#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct AudioData {
    std::string file_path;
    int sample_rate = 16000;
    int channels = 1;
    int64_t total_samples = 0;
    int64_t duration_ms = 0;
    std::vector<float> pcm_data;        // 16kHz mono float32 for Whisper
    std::vector<float> waveform_peaks;  // Normalized [0..1] peaks for UI waveform rendering
    bool is_loaded = false;
    std::string error_message;
};

class AudioDecoder {
public:
    // Decodes any audio or music format into 16kHz mono float32 PCM for Whisper
    static bool decode_to_whisper_pcm(const std::string& input_file, AudioData& out_audio, std::string* error_out = nullptr);

    // Uses FFmpeg CLI if available to convert any multimedia format to standard 16kHz mono WAV
    static bool run_ffmpeg_conversion(const std::string& input_path, const std::string& output_wav_path, std::string* error_out = nullptr);

    // Generates amplitude envelope peaks for real-time waveform display
    static void generate_waveform_peaks(const std::vector<float>& pcm, std::vector<float>& out_peaks, size_t num_bins = 512);
};
