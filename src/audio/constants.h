#pragma once
#include <vector>
#include <atomic>
#include <cstdint>

// Config
constexpr int FFT_SIZE = 2048;
constexpr int AUDIO_BUFFER_SAMPLES = 2048;
constexpr int CHANNELS = 2;
constexpr int NUM_BANDS = 16;
constexpr int INTERPOLATION_FACTOR = 4;
constexpr int VISUAL_BANDS = NUM_BANDS * INTERPOLATION_FACTOR; 
constexpr int MIRRORED_WIDTH = VISUAL_BANDS * 2; 
constexpr int VISUALIZER_HEIGHT = 32;

// Globals
extern std::vector<int16_t> g_raw_audio_buffer;
extern std::atomic<bool> g_new_audio_data_ready;
extern std::vector<float> g_fft_magnitudes;
extern std::atomic_bool quit_thread;
