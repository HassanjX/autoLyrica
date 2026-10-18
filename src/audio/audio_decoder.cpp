#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "audio_decoder.h"
#include "miniaudio.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <windows.h>

bool AudioDecoder::run_ffmpeg_conversion(const std::string& input_path, const std::string& output_wav_path, std::string* error_out) {
    // Construct ffmpeg command to extract 16kHz 1-channel 32-bit float or 16-bit PCM WAV
    std::string cmd = "ffmpeg.exe -y -i \"" + input_path + "\" -vn -ar 16000 -ac 1 -c:a pcm_f32le \"" + output_wav_path + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hidden console window
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmd_buffer(cmd.begin(), cmd.end());
    cmd_buffer.push_back('\0');

    if (!CreateProcessA(NULL, cmd_buffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        if (error_out) *error_out = "Failed to spawn ffmpeg.exe. Ensure FFmpeg is installed and added to PATH.";
        return false;
    }

    WaitForSingleObject(pi.hProcess, 120000); // 2 minute timeout for conversion
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exit_code != 0) {
        if (error_out) *error_out = "FFmpeg conversion failed with exit code " + std::to_string(exit_code);
        return false;
    }

    return true;
}

bool AudioDecoder::decode_to_whisper_pcm(const std::string& input_file, AudioData& out_audio, std::string* error_out) {
    out_audio.is_loaded = false;
    out_audio.pcm_data.clear();
    out_audio.waveform_peaks.clear();
    out_audio.file_path = input_file;

    // First attempt: try miniaudio native decoder directly (supports WAV, MP3, FLAC)
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 16000);
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(input_file.c_str(), &config, &decoder);

    std::string temp_wav_path = "";
    bool used_ffmpeg = false;

    if (result != MA_SUCCESS) {
        // Fallback: Use FFmpeg to convert to temporary 16kHz mono WAV
        char temp_dir[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_dir);
        temp_wav_path = std::string(temp_dir) + "autolyrica_temp_16k.wav";

        std::string ffmpeg_err;
        if (!run_ffmpeg_conversion(input_file, temp_wav_path, &ffmpeg_err)) {
            if (error_out) *error_out = "Decoder error: direct decode failed and FFmpeg conversion failed: " + ffmpeg_err;
            return false;
        }

        result = ma_decoder_init_file(temp_wav_path.c_str(), &config, &decoder);
        used_ffmpeg = true;

        if (result != MA_SUCCESS) {
            if (error_out) *error_out = "Failed to decode converted audio file (miniaudio error code: " + std::to_string(result) + ")";
            DeleteFileA(temp_wav_path.c_str());
            return false;
        }
    }

    // Read all samples
    ma_uint64 total_frames_to_read = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames_to_read);

    // If total length isn't known upfront, allocate dynamically in chunks
    if (total_frames_to_read > 0) {
        out_audio.pcm_data.resize(static_cast<size_t>(total_frames_to_read));
        ma_uint64 frames_read = 0;
        ma_decoder_read_pcm_frames(&decoder, out_audio.pcm_data.data(), total_frames_to_read, &frames_read);
        out_audio.pcm_data.resize(static_cast<size_t>(frames_read));
    } else {
        const size_t CHUNK_SIZE = 16000 * 2; // 2 seconds per chunk
        std::vector<float> chunk(CHUNK_SIZE);
        while (true) {
            ma_uint64 frames_read = 0;
            ma_result read_res = ma_decoder_read_pcm_frames(&decoder, chunk.data(), CHUNK_SIZE, &frames_read);
            if (frames_read > 0) {
                out_audio.pcm_data.insert(out_audio.pcm_data.end(), chunk.begin(), chunk.begin() + frames_read);
            }
            if (read_res != MA_SUCCESS || frames_read == 0) break;
        }
    }

    ma_decoder_uninit(&decoder);

    if (used_ffmpeg && !temp_wav_path.empty()) {
        DeleteFileA(temp_wav_path.c_str());
    }

    if (out_audio.pcm_data.empty()) {
        if (error_out) *error_out = "No audio data could be read from file.";
        return false;
    }

    out_audio.sample_rate = 16000;
    out_audio.channels = 1;
    out_audio.total_samples = static_cast<int64_t>(out_audio.pcm_data.size());
    out_audio.duration_ms = (out_audio.total_samples * 1000) / 16000;
    out_audio.is_loaded = true;

    // Generate waveform peaks for UI rendering
    generate_waveform_peaks(out_audio.pcm_data, out_audio.waveform_peaks, 512);

    return true;
}

void AudioDecoder::generate_waveform_peaks(const std::vector<float>& pcm, std::vector<float>& out_peaks, size_t num_bins) {
    out_peaks.clear();
    if (pcm.empty() || num_bins == 0) return;

    out_peaks.resize(num_bins, 0.0f);
    size_t samples_per_bin = std::max<size_t>(1, pcm.size() / num_bins);

    float max_val = 0.0001f;
    for (size_t bin = 0; bin < num_bins; ++bin) {
        size_t start = bin * samples_per_bin;
        size_t end = std::min(pcm.size(), start + samples_per_bin);
        float peak = 0.0f;
        for (size_t i = start; i < end; ++i) {
            float abs_val = std::abs(pcm[i]);
            if (abs_val > peak) peak = abs_val;
        }
        out_peaks[bin] = peak;
        if (peak > max_val) max_val = peak;
    }

    // Normalize peaks to [0.0, 1.0]
    for (float& p : out_peaks) {
        p = std::min(1.0f, p / max_val);
    }
}
