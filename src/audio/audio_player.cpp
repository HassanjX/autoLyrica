#ifndef NOMINMAX
#define NOMINMAX
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio_player.h"

#include <iostream>
#include <vector>
#include <windows.h>

struct AudioPlayerImpl {
    ma_engine engine;
    ma_sound sound;
    bool engine_initialized = false;
    bool sound_initialized = false;
    bool is_playing = false;
    std::string current_filepath;
    std::string temp_playback_wav;
    float volume = 0.85f;
};

AudioPlayer::AudioPlayer() : m_impl(std::make_unique<AudioPlayerImpl>()) {
    init();
}

AudioPlayer::~AudioPlayer() {
    uninit();
}

bool AudioPlayer::init() {
    if (m_impl->engine_initialized) return true;

    ma_result result = ma_engine_init(NULL, &m_impl->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio audio engine. Code: " << result << std::endl;
        return false;
    }

    m_impl->engine_initialized = true;
    return true;
}

void AudioPlayer::uninit() {
    if (m_impl->sound_initialized) {
        ma_sound_uninit(&m_impl->sound);
        m_impl->sound_initialized = false;
    }

    if (!m_impl->temp_playback_wav.empty()) {
        DeleteFileA(m_impl->temp_playback_wav.c_str());
        m_impl->temp_playback_wav.clear();
    }

    if (m_impl->engine_initialized) {
        ma_engine_uninit(&m_impl->engine);
        m_impl->engine_initialized = false;
    }
}

bool AudioPlayer::load_file(const std::string& filepath) {
    if (!m_impl->engine_initialized) {
        if (!init()) return false;
    }

    if (m_impl->sound_initialized) {
        ma_sound_stop(&m_impl->sound);
        ma_sound_uninit(&m_impl->sound);
        m_impl->sound_initialized = false;
    }

    if (!m_impl->temp_playback_wav.empty()) {
        DeleteFileA(m_impl->temp_playback_wav.c_str());
        m_impl->temp_playback_wav.clear();
    }

    m_impl->current_filepath = filepath;

    // First attempt: try miniaudio native loading (WAV, MP3, FLAC)
    ma_result result = ma_sound_init_from_file(&m_impl->engine, filepath.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &m_impl->sound);

    if (result != MA_SUCCESS) {
        // Fallback: Use FFmpeg to convert to temporary 44.1kHz stereo WAV for seamless playback of any format
        char temp_dir[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_dir);
        m_impl->temp_playback_wav = std::string(temp_dir) + "autolyrica_playback.wav";

        std::string cmd = "ffmpeg.exe -y -i \"" + filepath + "\" -vn -ar 44100 -ac 2 \"" + m_impl->temp_playback_wav + "\"";

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi, sizeof(pi));

        std::vector<char> cmd_buf(cmd.begin(), cmd.end());
        cmd_buf.push_back('\0');

        if (CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 120000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            result = ma_sound_init_from_file(&m_impl->engine, m_impl->temp_playback_wav.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &m_impl->sound);
        }
    }

    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load audio for playback: " << filepath << " (code: " << result << ")" << std::endl;
        return false;
    }

    m_impl->sound_initialized = true;
    ma_sound_set_volume(&m_impl->sound, m_impl->volume);
    return true;
}

void AudioPlayer::play() {
    if (!m_impl->sound_initialized) return;
    ma_sound_start(&m_impl->sound);
    m_impl->is_playing = true;
}

void AudioPlayer::pause() {
    if (!m_impl->sound_initialized) return;
    ma_sound_stop(&m_impl->sound);
    m_impl->is_playing = false;
}

void AudioPlayer::toggle_play() {
    if (!m_impl->sound_initialized) return;
    if (is_playing()) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::stop() {
    if (!m_impl->sound_initialized) return;
    ma_sound_stop(&m_impl->sound);
    ma_sound_seek_to_pcm_frame(&m_impl->sound, 0);
    m_impl->is_playing = false;
}

void AudioPlayer::seek_ms(int64_t ms) {
    if (!m_impl->sound_initialized) return;
    ma_uint32 sample_rate = 0;
    ma_sound_get_data_format(&m_impl->sound, NULL, NULL, &sample_rate, NULL, 0);
    if (sample_rate == 0) sample_rate = 44100;

    ma_uint64 target_frame = (static_cast<ma_uint64>(ms) * sample_rate) / 1000;
    ma_sound_seek_to_pcm_frame(&m_impl->sound, target_frame);
}

void AudioPlayer::set_volume(float volume) {
    m_impl->volume = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    if (m_impl->sound_initialized) {
        ma_sound_set_volume(&m_impl->sound, m_impl->volume);
    }
}

float AudioPlayer::get_volume() const {
    return m_impl->volume;
}

int64_t AudioPlayer::get_position_ms() const {
    if (!m_impl->sound_initialized) return 0;
    ma_uint32 sample_rate = 0;
    ma_sound_get_data_format(&m_impl->sound, NULL, NULL, &sample_rate, NULL, 0);
    if (sample_rate == 0) sample_rate = 44100;

    ma_uint64 cursor_frame = 0;
    ma_sound_get_cursor_in_pcm_frames(&m_impl->sound, &cursor_frame);
    return static_cast<int64_t>((cursor_frame * 1000) / sample_rate);
}

int64_t AudioPlayer::get_duration_ms() const {
    if (!m_impl->sound_initialized) return 0;
    ma_uint32 sample_rate = 0;
    ma_sound_get_data_format(&m_impl->sound, NULL, NULL, &sample_rate, NULL, 0);
    if (sample_rate == 0) sample_rate = 44100;

    ma_uint64 length_frames = 0;
    ma_sound_get_length_in_pcm_frames(&m_impl->sound, &length_frames);
    return static_cast<int64_t>((length_frames * 1000) / sample_rate);
}

bool AudioPlayer::is_playing() const {
    if (!m_impl->sound_initialized) return false;
    return ma_sound_is_playing(&m_impl->sound) == MA_TRUE;
}

bool AudioPlayer::is_loaded() const {
    return m_impl->sound_initialized;
}

std::string AudioPlayer::get_loaded_filepath() const {
    return m_impl->current_filepath;
}
