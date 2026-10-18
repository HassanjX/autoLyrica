#include "model_manager.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <windows.h>
#include <wininet.h>

ModelManager::ModelManager(const std::string& models_dir) {
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH) > 0) {
        std::string exe_str = exe_path;
        size_t last_slash = exe_str.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            std::string exe_dir = exe_str.substr(0, last_slash);
            m_models_dir = exe_dir + "/models";
        }
    }
    if (m_models_dir.empty()) {
        m_models_dir = models_dir;
    }

    // Create directory, fallback to %LOCALAPPDATA% if write restricted
    if (!CreateDirectoryA(m_models_dir.c_str(), NULL)) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            char appdata_path[MAX_PATH];
            if (GetEnvironmentVariableA("LOCALAPPDATA", appdata_path, MAX_PATH) > 0) {
                std::string user_base = std::string(appdata_path) + "/autoLyrica";
                CreateDirectoryA(user_base.c_str(), NULL);
                m_models_dir = user_base + "/models";
                CreateDirectoryA(m_models_dir.c_str(), NULL);
            }
        }
    }

    // Standard Hugging Face Whisper GGML repository models
    m_models = {
        {"base.en (Fast, Good)", "ggml-base.en.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin", "~142 MB", false, ""},
        {"small.en (Accurate)", "ggml-small.en.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin", "~466 MB", false, ""},
        {"large-v3-turbo (Best for Songs)", "ggml-large-v3-turbo.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3-turbo.bin", "~1.6 GB", false, ""},
        {"tiny.en (Ultra Fast)", "ggml-tiny.en.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin", "~75 MB", false, ""},
        {"base (Multilingual)", "ggml-base.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin", "~142 MB", false, ""},
        {"small (Multilingual)", "ggml-small.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin", "~466 MB", false, ""},
        {"medium.en (High Precision)", "ggml-medium.en.bin", "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin", "~1.5 GB", false, ""}
    };

    refresh_local_models();
}

ModelManager::~ModelManager() {
    cancel_download();
}

void ModelManager::refresh_local_models() {
    std::vector<std::string> candidate_dirs = { m_models_dir, "models", "../models", "../../models" };

    // Also add executable directory and %LOCALAPPDATA%
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH) > 0) {
        std::string exe_str = exe_path;
        size_t last_slash = exe_str.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            std::string exe_dir = exe_str.substr(0, last_slash);
            candidate_dirs.push_back(exe_dir + "/models");
            candidate_dirs.push_back(exe_dir);
            candidate_dirs.push_back(exe_dir + "/../../models");
        }
    }

    char appdata_path[MAX_PATH];
    if (GetEnvironmentVariableA("LOCALAPPDATA", appdata_path, MAX_PATH) > 0) {
        candidate_dirs.push_back(std::string(appdata_path) + "/autoLyrica/models");
        candidate_dirs.push_back(std::string(appdata_path) + "/autoLyrica");
    }

    for (auto& m : m_models) {
        bool found = false;
        for (const auto& dir : candidate_dirs) {
            std::string full_path = dir + "/" + m.filename;
            DWORD attr = GetFileAttributesA(full_path.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                WIN32_FILE_ATTRIBUTE_DATA fad;
                if (GetFileAttributesExA(full_path.c_str(), GetFileExInfoStandard, &fad)) {
                    LARGE_INTEGER size;
                    size.LowPart = fad.nFileSizeLow;
                    size.HighPart = fad.nFileSizeHigh;
                    if (size.QuadPart > 1024 * 1024) {
                        m.is_downloaded = true;
                        m.local_path = full_path;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            m.is_downloaded = false;
            m.local_path = "";
        }
    }
}


bool ModelManager::has_any_model() const {
    for (const auto& m : m_models) {
        if (m.is_downloaded) return true;
    }
    return false;
}

std::string ModelManager::get_default_model_path() const {
    // Prefer base.en or small.en or first available
    for (const auto& m : m_models) {
        if (m.is_downloaded) return m.local_path;
    }
    return "";
}

bool ModelManager::start_download(const std::string& model_name,
                                 std::function<void(float progress, const std::string& status)> progress_cb,
                                 std::function<void(bool success, const std::string& error)> complete_cb) {
    if (m_is_downloading) return false;

    auto it = std::find_if(m_models.begin(), m_models.end(), [&](const ModelInfo& mi) {
        return mi.name == model_name || mi.filename == model_name;
    });

    if (it == m_models.end()) {
        if (complete_cb) complete_cb(false, "Unknown model name: " + model_name);
        return false;
    }

    ModelInfo target_info = *it;
    m_is_downloading = true;
    m_cancel_requested = false;
    m_download_progress = 0.0f;
    m_download_status = "Connecting...";

    std::thread([this, target_info, progress_cb, complete_cb]() {
        HINTERNET hInternet = InternetOpenA("autoLyrica/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) {
            m_is_downloading = false;
            if (complete_cb) complete_cb(false, "Failed to initialize WinINet network session.");
            return;
        }

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE;
        HINTERNET hUrl = InternetOpenUrlA(hInternet, target_info.download_url.c_str(), NULL, 0, flags, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            m_is_downloading = false;
            if (complete_cb) complete_cb(false, "Could not connect to model repository URL.");
            return;
        }

        // Query content length
        DWORD content_length = 0;
        DWORD length_size = sizeof(content_length);
        DWORD header_idx = 0;
        HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &content_length, &length_size, &header_idx);

        CreateDirectoryA(m_models_dir.c_str(), NULL);
        std::string dest_path = m_models_dir + "/" + target_info.filename;
        std::string part_path = dest_path + ".part";

        std::ofstream out_file(part_path, std::ios::binary);
        if (!out_file.is_open()) {
            char appdata_path[MAX_PATH];
            if (GetEnvironmentVariableA("LOCALAPPDATA", appdata_path, MAX_PATH) > 0) {
                std::string user_base = std::string(appdata_path) + "/autoLyrica";
                CreateDirectoryA(user_base.c_str(), NULL);
                m_models_dir = user_base + "/models";
                CreateDirectoryA(m_models_dir.c_str(), NULL);
                dest_path = m_models_dir + "/" + target_info.filename;
                part_path = dest_path + ".part";
                out_file.open(part_path, std::ios::binary);
            }
        }

        if (!out_file.is_open()) {
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            m_is_downloading = false;
            if (complete_cb) complete_cb(false, "Failed to open target file for writing: " + part_path);
            return;
        }

        const DWORD BUFFER_SIZE = 64 * 1024;
        std::vector<char> buffer(BUFFER_SIZE);
        DWORD bytes_read = 0;
        DWORD total_read = 0;

        bool success = true;
        std::string err_msg = "";

        while (InternetReadFile(hUrl, buffer.data(), BUFFER_SIZE, &bytes_read) && bytes_read > 0) {
            if (m_cancel_requested) {
                success = false;
                err_msg = "Download cancelled by user.";
                break;
            }

            out_file.write(buffer.data(), bytes_read);
            total_read += bytes_read;

            float prog = (content_length > 0) ? (static_cast<float>(total_read) / content_length) : 0.0f;
            m_download_progress = prog;

            char status_buf[128];
            if (content_length > 0) {
                snprintf(status_buf, sizeof(status_buf), "Downloading: %.1f MB / %.1f MB (%.0f%%)",
                         total_read / (1024.0f * 1024.0f), content_length / (1024.0f * 1024.0f), prog * 100.0f);
            } else {
                snprintf(status_buf, sizeof(status_buf), "Downloading: %.1f MB...", total_read / (1024.0f * 1024.0f));
            }
            m_download_status = status_buf;

            if (progress_cb) {
                progress_cb(prog, m_download_status);
            }
        }

        out_file.close();
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);

        if (success && !m_cancel_requested) {
            MoveFileExA(part_path.c_str(), dest_path.c_str(), MOVEFILE_REPLACE_EXISTING);
            refresh_local_models();
            m_download_status = "Download complete!";
            m_download_progress = 1.0f;
            m_is_downloading = false;
            if (complete_cb) complete_cb(true, "");
        } else {
            DeleteFileA(part_path.c_str());
            m_download_status = err_msg.empty() ? "Download failed." : err_msg;
            m_is_downloading = false;
            if (complete_cb) complete_cb(false, m_download_status);
        }
    }).detach();

    return true;
}

void ModelManager::cancel_download() {
    if (m_is_downloading) {
        m_cancel_requested = true;
    }
}
