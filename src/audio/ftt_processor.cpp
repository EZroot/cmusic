#include "ftt_processor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
SimpleFFT::SimpleFFT() {
    cfg_ = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
    if (!cfg_) throw std::runtime_error("Failed to allocate kiss_fftr_cfg");
    output_size_ = (FFT_SIZE / 2) + 1;
    c_out_ = new kiss_fft_cpx[output_size_];
    r_in_.assign(FFT_SIZE, 0.0f);
}
SimpleFFT::~SimpleFFT() {
    if (cfg_) kiss_fftr_free(cfg_);
    delete[] c_out_;
}
void SimpleFFT::PerformFFT(const std::vector<float>& mono_samples, std::vector<float>& magnitude_bands) {
    size_t copy_n = std::min<size_t>(mono_samples.size(), r_in_.size());
    std::copy_n(mono_samples.data(), copy_n, r_in_.data());
    if (copy_n < r_in_.size()) std::fill(r_in_.begin() + copy_n, r_in_.end(), 0.0f);
    kiss_fftr(cfg_, r_in_.data(), c_out_);
    magnitude_bands.assign(NUM_BANDS, 0.0f);
    float scale_factor = 2.0f / static_cast<float>(FFT_SIZE);
    float log_step = 0.0f;
    if (output_size_ > 1) log_step = std::log10(static_cast<float>(output_size_)) / static_cast<float>(NUM_BANDS);
    for (int band = 0; band < NUM_BANDS; ++band) {
        int start_bin = 1;
        int end_bin = output_size_ - 1;
        if (output_size_ > 1) {
            start_bin = static_cast<int>(std::round(std::pow(10.0f, log_step * static_cast<float>(band))));
            end_bin = static_cast<int>(std::round(std::pow(10.0f, log_step * static_cast<float>(band + 1))));
            start_bin = std::clamp(start_bin, 1, output_size_ - 1);
            end_bin = std::clamp(end_bin, 1, output_size_ - 1);
            if (end_bin <= start_bin) end_bin = std::min(start_bin + 1, output_size_ - 1);
        }
        double sum_magnitude = 0.0;
        for (int k = start_bin; k < end_bin; ++k) {
            float r = c_out_[k].r;
            float i = c_out_[k].i;
            sum_magnitude += std::sqrt((double)r * r + (double)i * i);
        }
        int bins = std::max(1, end_bin - start_bin);
        double average_mag = sum_magnitude / static_cast<double>(bins);
        double visual_mag = 10.0 * std::log10(1.0 + 5000.0 * average_mag * scale_factor);
        double clamped = std::clamp(visual_mag / 40.0, 0.0, 1.0);
        magnitude_bands[band] = static_cast<float>(clamped);
    }
}