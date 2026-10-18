#pragma once

#include <string>
#include <cstdint>
#include <memory>

struct AudioPlayerImpl;

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool init();
    void uninit();

    bool load_file(const std::string& filepath);
    void play();
    void pause();
    void toggle_play();
    void stop();
    void seek_ms(int64_t ms);

    void set_volume(float volume); // 0.0f - 1.0f
    float get_volume() const;

    int64_t get_position_ms() const;
    int64_t get_duration_ms() const;
    bool is_playing() const;
    bool is_loaded() const;
    std::string get_loaded_filepath() const;

private:
    std::unique_ptr<AudioPlayerImpl> m_impl;
};
