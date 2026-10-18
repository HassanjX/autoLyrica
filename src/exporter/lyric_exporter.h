#pragma once

#include "transcriber/whisper_engine.h"
#include <string>
#include <vector>

enum class ExportFormat {
    EnhancedLRC, // Word-by-word karaoke LRC
    StandardLRC, // Line-by-line standard LRC
    SRT,         // SubRip subtitle
    VTT,         // WebVTT subtitle
    JSON,        // Detailed JSON with words & confidence
    TXT          // Plain text lyrics
};

class LyricExporter {
public:
    static std::string export_to_string(const std::vector<SegmentItem>& segments, ExportFormat format, const std::string& title = "autoLyrica");
    static bool export_to_file(const std::string& filepath, const std::vector<SegmentItem>& segments, ExportFormat format, const std::string& title = "autoLyrica");

    // Native Windows File Save Dialog
    static std::string open_save_dialog(ExportFormat format, const std::string& default_filename = "lyrics");

    static std::string format_lrc_timestamp(int64_t ms);
    static std::string format_srt_timestamp(int64_t ms);
    static std::string format_vtt_timestamp(int64_t ms);
};
