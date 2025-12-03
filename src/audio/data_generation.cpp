#include "data_generation.h"
#include <algorithm>
#include <cmath>
#include <ftxui/screen/color.hpp>
#include <stdexcept>

AudioDataGenerator::AudioDataGenerator() {}

static inline unsigned char LerpUC(float t, unsigned char a, unsigned char b) {
    return static_cast<unsigned char>(std::round(a * t + b * (1.0f - t)));
}

void AudioDataGenerator::generate_frame(std::vector<unsigned char>& data) {
    const int width_ = width();
    const int height_ = height();
    const size_t pixel_count = static_cast<size_t>(width_) * height_;
    data.resize(pixel_count * 3);

    const int num_bands = NUM_BANDS;
    const int visual_bands = VISUAL_BANDS;
    const int interpolation_factor = INTERPOLATION_FACTOR;

    if (g_fft_magnitudes.size() < static_cast<size_t>(num_bands)) {
        g_fft_magnitudes.assign(num_bands, 0.0f);
    }

    for (int x = 0; x < width_; ++x) {
        int visual_band_index = std::min(x, 2 * VISUAL_BANDS - 1 - x);

        visual_band_index = std::clamp(visual_band_index, 0, visual_bands - 1);
        
        int band_index = visual_band_index / interpolation_factor;
        int step = visual_band_index % interpolation_factor;

        band_index = std::clamp(band_index, 0, num_bands - 1);

        float mag1 = std::clamp(g_fft_magnitudes[band_index], 0.0f, 1.0f);
        
        float mag2 = mag1; 
        if (step > 0 && band_index + 1 < num_bands) {
            mag2 = std::clamp(g_fft_magnitudes[band_index + 1], 0.0f, 1.0f);
        }

        float t = 0.0f;
        if (interpolation_factor > 0) {
            t = static_cast<float>(step) / static_cast<float>(interpolation_factor);
        }
        float mag = mag1 * (1.0f - t) + mag2 * t; 

        int bar_height = static_cast<int>(std::round(mag * static_cast<float>(height_)));
        
        bar_height = std::clamp(bar_height, 0, height_);

        for (int y = 0; y < height_; ++y) {
            const size_t index = (static_cast<size_t>(y) * width_ + static_cast<size_t>(x)) * 3;

            if (y < height_ - bar_height) {
                data[index]     = 25; 
                data[index + 1] = 15; 
                data[index + 2] = 35; 
            } else {
                const int current_bar_height = height_ - y; 
                const int safe_bar_height = std::max(1, bar_height);

                float normalized = static_cast<float>(current_bar_height) / static_cast<float>(safe_bar_height);
                normalized = std::clamp(normalized, 0.0f, 1.0f);
                
                float curve = std::pow(normalized, 0.7f);

                data[index]     = LerpUC(curve, 255, 0);   
                data[index + 1] = LerpUC(curve, 0, 150);  
                data[index + 2] = LerpUC(curve, 180, 255);
            }
        }
    }
}

void MapFrameToCanvas(ftxui::Canvas& c, const std::vector<unsigned char>& data, int data_w, int data_h) {
    const int channels = 3;
    const int canvas_h = c.height();

    const int v_factor = std::max(1, data_h / canvas_h); 

    const size_t required_size = static_cast<size_t>(data_w) * data_h * channels;
    if (data.size() < required_size) {
        return; 
    }

    for (int y = 0; y < canvas_h; ++y) {
        const int start_y = y * v_factor;
        const int count = v_factor; 

        for (int x = 0; x < data_w; ++x) {
            long sum_R = 0, sum_G = 0, sum_B = 0;
            
            for (int k = 0; k < v_factor; ++k) {
                const int data_y = start_y + k;
                
                if (data_y >= data_h) {
                    break;
                }

                const size_t idx = (static_cast<size_t>(data_y) * data_w + static_cast<size_t>(x)) * channels;
                
                sum_R += data[idx];
                sum_G += data[idx + 1];
                sum_B += data[idx + 2];
            }
            
            int actual_count = std::min(v_factor, data_h - start_y);
            actual_count = std::max(1, actual_count); 

            c.DrawBlock(x, y, true,
                ftxui::Color::RGB(
                    static_cast<int>(sum_R / actual_count), 
                    static_cast<int>(sum_G / actual_count), 
                    static_cast<int>(sum_B / actual_count)
                ));
        }
    }
}