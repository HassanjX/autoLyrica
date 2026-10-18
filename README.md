# autoLyrica 🎵✨

<div align="center">

[![Release](https://img.shields.io/github/v/release/HassanjX/autoLyrica?color=blue&style=for-the-badge)](https://github.com/HassanjX/autoLyrica/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20x64-brightgreen.svg?style=for-the-badge)](#)
[![C++](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?style=for-the-badge)](#)
[![Whisper.cpp](https://img.shields.io/badge/Engine-whisper.cpp-ff69b4.svg?style=for-the-badge)](https://github.com/ggerganov/whisper.cpp)

**High-Performance Native C++ Desktop Application for Word-by-Word Audio & Song Lyrics Transcription**  
*Powered by `whisper.cpp`, Dear ImGui, DirectX 11, miniaudio, and AVX2/FMA vector acceleration.*

[Download Latest Release](https://github.com/HassanjX/autoLyrica/releases/latest) • [Key Features](#-key-features) • [Lyric Studio & Editor](#-built-in-lyric-studio--editor) • [Building from Source](#-building-from-source)

</div>

---

## 🌟 Key Features

- 🎯 **Pinpoint Word-by-Word Alignment**: Leverages Whisper token-level timestamps combined with Dynamic Time Warping (DTW) to compute the exact start and end millisecond boundaries for every word sung or spoken.
- ⚡ **Native C++ Performance**: Built with modern C++17, AVX2/FMA SIMD optimizations, and Dear ImGui + DirectX 11 for a smooth, fluid 60 FPS hardware-accelerated user experience with zero runtime lag.
- 🎧 **Universal Audio Ingestion**: Transcribe **any** audio or music file format:
  - `.mp3`, `.wav`, `.flac`, `.m4a`, `.ogg`, `.aac`, `.opus`, `.mp4`, `.webm`, `.wma`
  - Ultra-fast native decoding via `miniaudio` with automatic multi-channel downmixing and resampling to 16 kHz mono float32.
- 🎤 **Synchronized Interactive Karaoke Player**:
  - Live karaoke lyrics follow the music playback in real-time.
  - Active line and individual sung words highlight dynamically in radiant gold (`#FDCB6E`).
  - **Click-to-Seek**: Click on any word in the transcript to jump audio playback directly to that exact moment.
  - Interactive audio waveform scrubber with playhead tracking and volume controls.
- ✏️ **Built-in Lyric Studio & Word Editor**:
  - Full in-place editing for line text with automated word-token resynchronization.
  - Interactive word badges displaying exact timestamps (`[mm:ss.xx]`).
  - Word Editor Modal with +/- 100ms and +/- 500ms nudges, "🎯 Set to Now", and audio preview.
  - Global `Shift All` offset tools for aligning the entire song at once.
- 💾 **Comprehensive Multi-Format Export**:
  - **Enhanced LRC (Karaoke)**: `[00:15.20]<00:15.20> And <00:15.65> so <00:16.10> my ...` (industry-standard format supported by modern karaoke players).
  - **Standard LRC**: Line-by-line synchronized lyrics `[mm:ss.xx] Line`.
  - **SubRip Subtitles (.srt)**: Full standard subtitle format for video editors.
  - **WebVTT (.vtt)**: Web standard video/audio subtitles.
  - **Detailed JSON**: Complete structured metadata containing word text, start/end milliseconds, and confidence ratings.
  - **Plain Text (.txt)**.
- 📥 **Integrated Model Manager**:
  - One-click downloader for official Whisper GGML models directly inside the application (`tiny`, `base`, `small`, `medium`, `large-v3-turbo`) with real-time download progress.
  - Automatic detection of models placed in `./models/` or alongside the executable.
- 🖱️ **Drag & Drop**: Simply drag any music or voice recording from Windows File Explorer onto the app window.

---

## 🎛️ Built-in Lyric Studio & Editor

autoLyrica includes a full editing suite to review and polish generated lyrics:

| Feature | Description |
| :--- | :--- |
| **`🎯 Set to Now`** | Aligns the line or word start timestamp directly to the current music playback position. |
| **Time Nudges** | Quick nudge buttons (`-0.5s`, `-0.1s`, `+0.1s`, `+0.5s`) for adjusting timing on the fly. |
| **Global Shift** | Header bar with `Shift All: -0.5s / -0.1s / +0.1s / +0.5s` to adjust all lines simultaneously. |
| **Word Editor Modal** | Click any word badge to fine-tune spelling, adjust start/end times in milliseconds, or preview the specific audio snippet. |
| **Line Operations** | Insert lines below (`+ Below`), merge lines (`Merge ↓`), or remove lines (`✕`). |
| **Auto-Sync** | Automatically preserves exact DTW word timestamps when editing spelling. |

---

## 📦 Download & Quick Start

### 1. Download Precompiled Executable
Head over to the [**GitHub Releases**](https://github.com/HassanjX/autoLyrica/releases/latest) page and download:
* **`autoLyrica-Installer.exe`**: Complete standalone bundle including the application and the standard Whisper models (`ggml-base.en.bin` & `ggml-tiny.en.bin`).
* Or download the lightweight portable folder package.

### 2. Run
Double-click `autoLyrica.exe`. Drag and drop any audio file, select your Whisper model, and click **START TRANSCRIBE**.

---

## 🏗️ Architecture & Codebase

```
autoLyrica/
├── CMakeLists.txt                 # Master CMake build configuration
├── build.bat                      # 1-click build script (Release x64)
├── run.bat                        # 1-click launcher script
├── models/                        # Whisper ggml models directory
│   ├── ggml-base.en.bin           # Whisper Base English model
│   └── ggml-tiny.en.bin           # Whisper Tiny English model
├── third_party/
│   ├── whisper.cpp/               # Georgi Gerganov's high-performance Whisper engine
│   ├── imgui/                     # Dear ImGui + DirectX11 / Win32 backends
│   └── miniaudio/                 # miniaudio.h (WASAPI playback & audio decoders)
├── src/
│   ├── main.cpp                   # Win32 window, DX11 swap chain, message pump
│   ├── audio/
│   │   ├── audio_decoder.h / .cpp # Universal audio decoder & waveform peak extractor
│   │   └── audio_player.h / .cpp  # WASAPI real-time audio playback & seek engine
│   ├── transcriber/
│   │   ├── whisper_engine.h / .cpp# DTW token timestamp extraction & worker threads
│   │   └── model_manager.h / .cpp # Model scanner & WinINet background downloader
│   ├── ui/
│   │   ├── app_ui.h / .cpp        # Dashboard layout, controls, and waveform bar
│   │   ├── lyrics_view.h / .cpp   # Interactive word-by-word karaoke studio
│   │   └── theme.h / .cpp         # Custom modern dark theme
│   └── exporter/
│       └── lyric_exporter.h / .cpp# Enhanced LRC, Standard LRC, SRT, VTT, JSON, TXT
└── tests/
    └── test_pipeline.cpp          # Automated pipeline verification suite
```

---

## 🛠️ Building from Source

### Prerequisites
* Windows 10 or 11 (64-bit)
* Visual Studio 2022 (Community or higher) with **Desktop development with C++**
* CMake 3.18 or newer
* Git

### Build Instructions

1. **Clone the repository with submodules:**
   ```bash
   git clone --recurse-submodules https://github.com/HassanjX/autoLyrica.git
   cd autoLyrica
   ```

2. **Configure and Build:**
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
   Or simply run:
   ```cmd
   build.bat
   ```

3. **Run:**
   ```powershell
   .\build\Release\autoLyrica.exe
   ```

---

## 📄 License

This project is open-source under the [MIT License](LICENSE).
Feel free to use, modify, and distribute it for personal and commercial projects.
