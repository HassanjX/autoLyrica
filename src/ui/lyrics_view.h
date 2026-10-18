#pragma once

#include "transcriber/whisper_engine.h"
#include "audio/audio_player.h"

#include <vector>
#include <string>

class LyricsView {
public:
    LyricsView();
    ~LyricsView();

    void render(std::vector<SegmentItem>& segments, AudioPlayer& player, int64_t current_time_ms);

    bool is_edit_mode() const { return m_edit_mode; }
    void set_edit_mode(bool edit) { m_edit_mode = edit; }

    bool is_auto_scroll_enabled() const { return m_auto_scroll; }
    void set_auto_scroll(bool auto_scroll) { m_auto_scroll = auto_scroll; }

    // Resynchronizes word tokens when line text is edited
    static void sync_words_to_text(SegmentItem& seg);

    // Shifts all segment and word timestamps by an offset in milliseconds
    static void shift_all_timestamps(std::vector<SegmentItem>& segments, int64_t offset_ms);

private:
    void render_word_editor_modal(std::vector<SegmentItem>& segments, AudioPlayer& player);

    bool m_edit_mode = false;
    bool m_auto_scroll = true;
    int m_last_active_segment_id = -1;

    // Word Editor Modal State
    int m_edit_seg_idx = -1;
    int m_edit_word_idx = -1;
    char m_word_edit_buf[128] = { 0 };
    int64_t m_word_edit_start = 0;
    int64_t m_word_edit_end = 0;
    bool m_open_word_modal = false;
};
