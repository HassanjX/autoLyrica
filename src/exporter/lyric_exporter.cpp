#include "lyric_exporter.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <commdlg.h>

std::string LyricExporter::format_lrc_timestamp(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t total_sec = ms / 1000;
    int64_t csec = (ms % 1000) / 10; // centiseconds (00-99)
    int64_t mins = total_sec / 60;
    int64_t secs = total_sec % 60;

    std::ostringstream ss;
    ss << "[" << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs << "."
       << std::setfill('0') << std::setw(2) << csec << "]";
    return ss.str();
}

static std::string format_enhanced_tag(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t total_sec = ms / 1000;
    int64_t csec = (ms % 1000) / 10;
    int64_t mins = total_sec / 60;
    int64_t secs = total_sec % 60;

    std::ostringstream ss;
    ss << "<" << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs << "."
       << std::setfill('0') << std::setw(2) << csec << ">";
    return ss.str();
}

std::string LyricExporter::format_srt_timestamp(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t total_sec = ms / 1000;
    int64_t msec = ms % 1000;
    int64_t hours = total_sec / 3600;
    int64_t mins = (total_sec % 3600) / 60;
    int64_t secs = total_sec % 60;

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs << ","
       << std::setfill('0') << std::setw(3) << msec;
    return ss.str();
}

std::string LyricExporter::format_vtt_timestamp(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t total_sec = ms / 1000;
    int64_t msec = ms % 1000;
    int64_t hours = total_sec / 3600;
    int64_t mins = (total_sec % 3600) / 60;
    int64_t secs = total_sec % 60;

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs << "."
       << std::setfill('0') << std::setw(3) << msec;
    return ss.str();
}

static std::string escape_json_string(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::string LyricExporter::export_to_string(const std::vector<SegmentItem>& segments, ExportFormat format, const std::string& title) {
    std::ostringstream out;

    switch (format) {
        case ExportFormat::EnhancedLRC: {
            out << "[ti:" << title << "]\n";
            out << "[re:autoLyrica C++]\n";
            for (const auto& seg : segments) {
                out << format_lrc_timestamp(seg.start_ms);
                if (!seg.words.empty()) {
                    for (const auto& word : seg.words) {
                        out << format_enhanced_tag(word.start_ms) << " " << word.text << " ";
                    }
                } else {
                    out << " " << seg.text;
                }
                out << "\n";
            }
            break;
        }

        case ExportFormat::StandardLRC: {
            out << "[ti:" << title << "]\n";
            out << "[re:autoLyrica C++]\n";
            for (const auto& seg : segments) {
                out << format_lrc_timestamp(seg.start_ms) << " " << seg.text << "\n";
            }
            break;
        }

        case ExportFormat::SRT: {
            int idx = 1;
            for (const auto& seg : segments) {
                out << idx++ << "\n";
                out << format_srt_timestamp(seg.start_ms) << " --> " << format_srt_timestamp(seg.end_ms) << "\n";
                out << seg.text << "\n\n";
            }
            break;
        }

        case ExportFormat::VTT: {
            out << "WEBVTT\n\n";
            for (const auto& seg : segments) {
                out << format_vtt_timestamp(seg.start_ms) << " --> " << format_vtt_timestamp(seg.end_ms) << "\n";
                out << seg.text << "\n\n";
            }
            break;
        }

        case ExportFormat::JSON: {
            out << "{\n";
            out << "  \"title\": \"" << escape_json_string(title) << "\",\n";
            out << "  \"generator\": \"autoLyrica\",\n";
            out << "  \"segments\": [\n";
            for (size_t i = 0; i < segments.size(); ++i) {
                const auto& seg = segments[i];
                out << "    {\n";
                out << "      \"id\": " << seg.id << ",\n";
                out << "      \"start_ms\": " << seg.start_ms << ",\n";
                out << "      \"end_ms\": " << seg.end_ms << ",\n";
                out << "      \"text\": \"" << escape_json_string(seg.text) << "\",\n";
                out << "      \"words\": [\n";
                for (size_t w = 0; w < seg.words.size(); ++w) {
                    const auto& word = seg.words[w];
                    out << "        {\"text\": \"" << escape_json_string(word.text) 
                        << "\", \"start_ms\": " << word.start_ms 
                        << ", \"end_ms\": " << word.end_ms 
                        << ", \"confidence\": " << std::fixed << std::setprecision(3) << word.confidence << "}";
                    if (w + 1 < seg.words.size()) out << ",";
                    out << "\n";
                }
                out << "      ]\n";
                out << "    }";
                if (i + 1 < segments.size()) out << ",";
                out << "\n";
            }
            out << "  ]\n";
            out << "}\n";
            break;
        }

        case ExportFormat::TXT: {
            for (const auto& seg : segments) {
                out << seg.text << "\n";
            }
            break;
        }
    }

    return out.str();
}

bool LyricExporter::export_to_file(const std::string& filepath, const std::vector<SegmentItem>& segments, ExportFormat format, const std::string& title) {
    std::string content = export_to_string(segments, format, title);
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(content.data(), content.size());
    return true;
}

std::string LyricExporter::open_save_dialog(ExportFormat format, const std::string& default_filename) {
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = { 0 };

    std::wstring def_name(default_filename.begin(), default_filename.end());
    wcsncpy_s(szFile, def_name.c_str(), _TRUNCATE);

    const wchar_t* filter = L"All Files (*.*)\0*.*\0";
    const wchar_t* def_ext = L"txt";

    switch (format) {
        case ExportFormat::EnhancedLRC:
        case ExportFormat::StandardLRC:
            filter = L"LRC Lyrics (*.lrc)\0*.lrc\0All Files (*.*)\0*.*\0";
            def_ext = L"lrc";
            break;
        case ExportFormat::SRT:
            filter = L"SubRip Subtitles (*.srt)\0*.srt\0All Files (*.*)\0*.*\0";
            def_ext = L"srt";
            break;
        case ExportFormat::VTT:
            filter = L"WebVTT Subtitles (*.vtt)\0*.vtt\0All Files (*.*)\0*.*\0";
            def_ext = L"vtt";
            break;
        case ExportFormat::JSON:
            filter = L"JSON Metadata (*.json)\0*.json\0All Files (*.*)\0*.*\0";
            def_ext = L"json";
            break;
        case ExportFormat::TXT:
            filter = L"Plain Text (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
            def_ext = L"txt";
            break;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = def_ext;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn) == TRUE) {
        int len = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            std::string path(len - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &path[0], len, NULL, NULL);
            return path;
        }
    }
    return "";
}
