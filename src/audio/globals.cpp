#include "constants.h"

std::vector<int16_t> g_raw_audio_buffer(AUDIO_BUFFER_SAMPLES * CHANNELS, 0);
std::atomic<bool> g_new_audio_data_ready(false);
std::vector<float> g_fft_magnitudes(NUM_BANDS, 0.0f);
std::atomic_bool quit_thread(false);
