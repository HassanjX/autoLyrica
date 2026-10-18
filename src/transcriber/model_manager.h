#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <functional>

struct ModelInfo {
    std::string name;
    std::string filename;
    std::string download_url;
    std::string size_desc;
    bool is_downloaded = false;
    std::string local_path;
};

class ModelManager {
public:
    ModelManager(const std::string& models_dir = "models");
    ~ModelManager();

    void refresh_local_models();
    const std::vector<ModelInfo>& get_available_models() const { return m_models; }

    bool has_any_model() const;
    std::string get_default_model_path() const;

    // Asynchronous model download with progress reporting
    bool start_download(const std::string& model_name, 
                        std::function<void(float progress, const std::string& status)> progress_cb,
                        std::function<void(bool success, const std::string& error)> complete_cb);

    void cancel_download();
    bool is_downloading() const { return m_is_downloading; }
    float get_download_progress() const { return m_download_progress; }
    std::string get_download_status() const { return m_download_status; }

private:
    std::string m_models_dir;
    std::vector<ModelInfo> m_models;
    std::atomic<bool> m_is_downloading{false};
    std::atomic<bool> m_cancel_requested{false};
    std::atomic<float> m_download_progress{0.0f};
    std::string m_download_status;
};
