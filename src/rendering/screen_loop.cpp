#include "timer.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/color.hpp>
#include <thread>
#include <iostream>
#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <SDL2/SDL_mixer.h>
#include <kissfft/kiss_fft.h>
#include <kissfft/kiss_fftr.h>
#include <complex>


constexpr int FFT_SIZE = 2048;
constexpr int AUDIO_BUFFER_SAMPLES = 2048;
constexpr int CHANNELS = 2;
constexpr int NUM_BANDS = 16;

extern std::vector<int16_t> g_raw_audio_buffer;
extern std::atomic<bool> g_new_audio_data_ready;
extern std::vector<float> g_fft_magnitudes;

class SimpleFFT {
public:
    SimpleFFT() {
        cfg_ = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
        output_size_ = (FFT_SIZE / 2) + 1;
        c_out_ = new kiss_fft_cpx[output_size_];
        r_in_.assign(FFT_SIZE, 0.0f);
    }

    ~SimpleFFT() {
        if (cfg_) kiss_fftr_free(cfg_);
        delete[] c_out_;
    }

    void PerformFFT(const std::vector<float>& mono_samples, std::vector<float>& magnitude_bands) {
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

private:
    kiss_fftr_cfg cfg_ = nullptr;
    kiss_fft_cpx* c_out_ = nullptr;
    std::vector<float> r_in_;
    int output_size_ = 0;
};

using namespace ftxui;
std::atomic_bool quit_thread = false;

class AudioDataGenerator {
public:
    AudioDataGenerator()
        : width_(NUM_BANDS), height_(64), canvas_width_(NUM_BANDS), canvas_height_(64) {}

    void generate_frame(std::vector<unsigned char>& data) {
        data.resize(width_ * height_ * 3);
        if (g_fft_magnitudes.size() < static_cast<size_t>(width_)) g_fft_magnitudes.assign(width_, 0.0f);

        for (int x = 0; x < width_; ++x) {
            float mag = std::clamp(g_fft_magnitudes[x], 0.0f, 1.0f);
            int bar_height = static_cast<int>(std::round(mag * static_cast<float>(height_)));
            for (int y = 0; y < height_; ++y) {
                long index = (static_cast<long>(y) * width_ + x) * 3;
                unsigned char R = 0, G = 0, B = 0;
                if (y < height_ - bar_height) {
                    R = G = B = 5;
                } else {
                    int effective_bar = std::max(1, bar_height);
                    float normalized_bar_pos = static_cast<float>(height_ - y) / static_cast<float>(effective_bar);
                    normalized_bar_pos = std::clamp(normalized_bar_pos, 0.0f, 1.0f);
                    R = static_cast<unsigned char>(255.0f * normalized_bar_pos);
                    G = static_cast<unsigned char>(255.0f * (1.0f - normalized_bar_pos));
                    B = 0;
                }
                data[index] = R;
                data[index + 1] = G;
                data[index + 2] = B;
            }
        }
    }

    int width() const { return width_; }
    int height() const { return height_; }
    int canvas_width() const { return canvas_width_; }
    int canvas_height() const { return canvas_height_; }

private:
    int width_;
    int height_;
    int canvas_width_;
    int canvas_height_;
};

void RedrawThread(ftxui::ScreenInteractive* screen_ptr, float target_fps) {
    using namespace std::chrono;
    milliseconds delay_ms = milliseconds(static_cast<int>(std::round(1000.0f / target_fps)));
    while (!quit_thread) {
        std::this_thread::sleep_for(delay_ms);
        screen_ptr->PostEvent(ftxui::Event::Custom);
    }
}

auto MapFrameToCanvas = [](ftxui::Canvas& c, const std::vector<unsigned char>& data, int data_w, int data_h) {
    const int channels = 3;
    const int canvas_h = c.height();
    int v_compression_factor = data_h / canvas_h;
    if (v_compression_factor <= 0) v_compression_factor = 1;
    for (int canvas_y = 0; canvas_y < canvas_h; ++canvas_y) {
        int data_y_start = canvas_y * v_compression_factor;
        for (int x = 0; x < data_w; ++x) {
            long sum_R = 0;
            long sum_G = 0;
            long sum_B = 0;
            int samples = 0;
            for (int k = 0; k < v_compression_factor; ++k) {
                int data_y = data_y_start + k;
                if (data_y < data_h) {
                    long index = (static_cast<long>(data_y) * data_w + x) * channels;
                    if (index + 2 < static_cast<long>(data.size())) {
                        sum_R += data[index];
                        sum_G += data[index + 1];
                        sum_B += data[index + 2];
                        ++samples;
                    }
                }
            }
            if (samples == 0) samples = 1;
            unsigned char avg_R = static_cast<unsigned char>(sum_R / samples);
            unsigned char avg_G = static_cast<unsigned char>(sum_G / samples);
            unsigned char avg_B = static_cast<unsigned char>(sum_B / samples);
            const ftxui::Color color_value = ftxui::Color::RGB(static_cast<int>(avg_R), static_cast<int>(avg_G), static_cast<int>(avg_B));
            c.DrawPoint(x, canvas_y, true, color_value);
        }
    }
};

void RunAppLoop(const float FPS_TARGET) {
    float total_elapsed_time = 0.0f;
    TimePoint last_time = Clock::now();
    std::string status_message = "Status: Running Visualizer...";

    SimpleFFT fft_processor;
    AudioDataGenerator data_generator;
    std::vector<unsigned char> current_frame_pixels;
    int w = data_generator.width();
    int h = data_generator.height();
    int canvas_width = data_generator.canvas_width();
    int canvas_height = data_generator.canvas_height();

    if (g_fft_magnitudes.size() < static_cast<size_t>(NUM_BANDS)) g_fft_magnitudes.assign(NUM_BANDS, 0.0f);

    auto screen = ScreenInteractive::TerminalOutput();
    std::thread redraw_thread(RedrawThread, &screen, FPS_TARGET);

    auto renderer = Renderer([&]{
        TimePoint current_time = Clock::now();
        Duration delta_duration = current_time - last_time;
        float delta_time = delta_duration.count();
        if (delta_time <= 0.0f) delta_time = 1.0f / FPS_TARGET;
        last_time = current_time;
        total_elapsed_time += delta_time;

        if (g_new_audio_data_ready.exchange(false)) {
            std::vector<float> mono_samples(AUDIO_BUFFER_SAMPLES, 0.0f);
            size_t available_frames = g_raw_audio_buffer.size() / std::max(1, CHANNELS);
            size_t copy_n = std::min<size_t>(mono_samples.size(), available_frames);
            for (size_t i = 0; i < copy_n; ++i) {
                mono_samples[i] = static_cast<float>(g_raw_audio_buffer[i * CHANNELS]) / 32768.0f;
            }
            if (copy_n < mono_samples.size()) std::fill(mono_samples.begin() + copy_n, mono_samples.end(), 0.0f);
            fft_processor.PerformFFT(mono_samples, g_fft_magnitudes);
        }

        data_generator.generate_frame(current_frame_pixels);

        auto c = Canvas(canvas_width, canvas_height);
        MapFrameToCanvas(c, current_frame_pixels, w, h);

        status_message = "Status: " + std::to_string(static_cast<int>(std::round(1.0f / delta_time))) + " FPS (Actual) | Audio Playing: " + (Mix_PlayingMusic() ? "YES" : "NO");
        std::string dt_text = "Delta(T): " + std::to_string(delta_time) + "s";
        std::string total_dt_text = "Elapsed(T): " + std::to_string(total_elapsed_time) + "s";

        auto canvas_element = canvas(c) | border;
        auto status_bar = text("(Target FPS: " + std::to_string(FPS_TARGET) + ") | " + status_message) | bold;
        auto info_bar = hbox({
            text(dt_text) | color(Color::Yellow),
            filler(),
            text(total_dt_text) | color(Color::Yellow),
        });

        return vbox({
            status_bar,
            separator(),
            canvas_element | flex,
            separator(),
            info_bar,
        });
    });

    screen.Loop(renderer);

    quit_thread = true;
    if (redraw_thread.joinable()) redraw_thread.join();
    std::cout << "Application loop finished. Total elapsed time: " << total_elapsed_time << "s" << std::endl;
}
