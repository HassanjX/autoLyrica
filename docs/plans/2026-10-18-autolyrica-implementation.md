# autoLyrica Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a high-performance C++ desktop application (**autoLyrica**) powered by Whisper (`whisper.cpp`) and Dear ImGui + DirectX 11 that takes any audio or music file, decodes it universally via FFmpeg/miniaudio, transcribes it word-by-word with precise timestamps using DTW token alignment, and provides a synchronized karaoke audio player, in-place lyric editor, and multi-format exporter (Enhanced LRC, SRT, VTT, JSON, TXT).

**Architecture:** 
- **Core Engine:** `whisper.cpp` for ultra-fast C++ inference with DTW (dynamic time warping) token-level timestamps, auto-detecting CUDA GPU acceleration on the NVIDIA GTX 1050 Ti with seamless CPU AVX2 fallback.
- **Audio Pipeline:** Universal audio ingestion using `ffmpeg` and `miniaudio.h` to convert any audio/music format (MP3, WAV, FLAC, M4A, OGG, AAC, etc.) into 16kHz mono 32-bit float PCM for Whisper, while streaming original audio through WASAPI for synchronized playback.
- **Desktop UI & Visualizer:** Dear ImGui with DirectX 11 / Win32 backend, rendering 60 FPS synchronized word-by-word karaoke highlighting, click-to-seek waveform/timeline, drag-and-drop file support, model manager, and lyric editing.

**Tech Stack:**
- Language: C++17
- Build System: CMake (supporting MSVC 2022 & MinGW-w64)
- GUI: Dear ImGui (v1.91+) + DirectX 11 + Win32
- Inference: `whisper.cpp` (Georgi Gerganov) + GGML (CUDA / AVX2)
- Audio: `miniaudio.h` (playback & decoding) + FFmpeg (universal format conversion)
- OS: Windows 10/11 x64

---

### Task 1: Project Scaffold & CMake Setup
**Files:**
- Create: `CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `third_party/` vendored / submodule setup

**Step 1:** Create `CMakeLists.txt` configuring C++17, Win32 / DX11 dependencies (`d3d11.lib`, `dxgi.lib`, `d3dcompiler.lib`), include directories, and Whisper / ImGui / miniaudio build targets.
**Step 2:** Test-build a minimal DirectX 11 Win32 window to verify compiler and DX11 linking.
**Step 3:** Commit the project scaffold.

---

### Task 2: Dear ImGui & DirectX 11 Modern UI Shell
**Files:**
- Create: `src/ui/app_ui.h`, `src/ui/app_ui.cpp`
- Create: `src/ui/theme.h`, `src/ui/theme.cpp`
- Setup: `third_party/imgui/` (imgui core + `imgui_impl_win32` + `imgui_impl_dx11`)

**Step 1:** Setup ImGui files and modern dark sleek styling (custom palette, rounded corners, responsive typography).
**Step 2:** Implement main application loop in `src/main.cpp` with DirectX 11 swapchain, DPI awareness, and window resizing.
**Step 3:** Build and run to verify the UI shell renders smoothly at 60 FPS.
**Step 4:** Commit UI foundation.

---

### Task 3: Universal Audio Ingestion & Decoding Pipeline
**Files:**
- Create: `src/audio/audio_decoder.h`, `src/audio/audio_decoder.cpp`
- Setup: `third_party/miniaudio/miniaudio.h`

**Step 1:** Implement `AudioDecoder` that accepts any audio file path (MP3, WAV, FLAC, M4A, AAC, OGG, etc.).
**Step 2:** Use `miniaudio` for direct decoding where possible, and spawn system `ffmpeg` (`-ar 16000 -ac 1 -c:a pcm_f32le`) for universal container support.
**Step 3:** Convert audio into `std::vector<float>` (16kHz mono float32) normalized to [-1.0, 1.0] for Whisper.
**Step 4:** Write automated unit verification to load a test audio sample and verify duration, sample rate, and channels.
**Step 5:** Commit audio decoder.

---

### Task 4: Whisper.cpp Integration & Word-Level Timestamp Extraction
**Files:**
- Setup: `third_party/whisper.cpp`
- Create: `src/transcriber/whisper_engine.h`, `src/transcriber/whisper_engine.cpp`
- Create: `src/transcriber/model_manager.h`, `src/transcriber/model_manager.cpp`

**Step 1:** Configure `whisper.cpp` with DTW token timestamps (`whisper_full_params::token_timestamps = true`, `dtw_token_timestamps = true`).
**Step 2:** Implement `ModelManager` to detect models in `models/` directory and download standard models (`tiny`, `base`, `small`, `large-v3-turbo`) with progress feedback.
**Step 3:** Implement `WhisperEngine::transcribe(...)` running in a worker thread so the UI never freezes.
**Step 4:** Extract detailed timestamps: `Segment` containing `Word` list (`word.text`, `word.start_ms`, `word.end_ms`, `word.confidence`).
**Step 5:** Verify transcription on a real audio sample.
**Step 6:** Commit Whisper engine.

---

### Task 5: Synchronized Audio Player with Real-Time Karaoke Highlighting
**Files:**
- Create: `src/audio/audio_player.h`, `src/audio/audio_player.cpp`
- Create: `src/ui/lyrics_view.h`, `src/ui/lyrics_view.cpp`

**Step 1:** Build `AudioPlayer` wrapping `miniaudio` device playback (WASAPI), providing `play()`, `pause()`, `seek(ms)`, `get_position_ms()`, and volume controls.
**Step 2:** Implement `LyricsView`:
  - Visual display of transcript with current active line and active word highlighted in real-time.
  - Interactive click-to-seek: clicking any word seeks audio directly to `word.start_ms`.
  - In-place editing: double click any word or line to fix lyrics or adjust timestamps.
**Step 3:** Test playback sync with sample audio and verify accurate word tracking.
**Step 4:** Commit synchronized player.

---

### Task 6: Multi-Format Exporter (Enhanced LRC, SRT, VTT, JSON, TXT)
**Files:**
- Create: `src/exporter/lyric_exporter.h`, `src/exporter/lyric_exporter.cpp`

**Step 1:** Implement Enhanced LRC generation with word-by-word karaoke tags:
  `[00:15.20]<00:15.20> Never <00:15.60> gonna <00:16.00> give <00:16.40> you <00:16.70> up`
**Step 2:** Implement SubRip (`.srt`) and WebVTT (`.vtt`) exporters with word pacing or line pacing.
**Step 3:** Implement JSON exporter with full metadata (words, start/end ms, confidence).
**Step 4:** Add Export buttons in the UI with a native Windows file save dialog.
**Step 5:** Commit exporter.

---

### Task 7: Drag & Drop, Model Selector, & Polish
**Files:**
- Modify: `src/ui/app_ui.cpp`
- Modify: `src/main.cpp`

**Step 1:** Implement native Windows Drag & Drop (`WM_DROPFILES`) so users can drop any song directly onto the app window.
**Step 2:** Add Model Selection combo box and Download Model progress modal.
**Step 3:** Add waveform / progress bar seeker with scrubbing.
**Step 4:** End-to-end integration test with a sample music file.
**Step 5:** Final documentation & release build script (`build.bat`).
