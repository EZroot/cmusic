#include "constants.h"
#include "ftt_processor.h"
#include "data_generation.h"
#include "timer.h" 

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/color.hpp>

#include <SDL2/SDL_mixer.h> 

#include <thread>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

void RedrawThread(ftxui::ScreenInteractive* screen_ptr, float target_fps) {
    using namespace std::chrono;
    milliseconds delay_ms = milliseconds(static_cast<int>(std::round(1000.0f / target_fps)));
    while (!quit_thread) {
        std::this_thread::sleep_for(delay_ms);
        screen_ptr->PostEvent(ftxui::Event::Custom);
    }
}

void RunAppLoop(const float FPS_TARGET) {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<float>;
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
    auto screen = ftxui::ScreenInteractive::TerminalOutput();

    // menu
    std::vector<std::string> menu_entries = {
        "(p) Play/Pause",
        "(n) Next Song",
        "(l) Load File",
        "(s) Settings",
        "(q) Quit"
    };

    int menu_selected = 0;
    std::string menu_feedback = "Ready";
    std::vector<ftxui::Component> menu_buttons;
    for (auto& entry : menu_entries) {
        menu_buttons.push_back(ftxui::Button(entry, [&, entry] {
            menu_feedback = "Action: " + entry;
            if (entry == "(q) Quit") {
                quit_thread = true;
                screen.Exit();
            } else if (entry == "(p) Play/Pause") {
                if (Mix_PausedMusic()) Mix_ResumeMusic();
                else if (Mix_PlayingMusic()) Mix_PauseMusic();
            }
        }));
    }
    auto menu_component = ftxui::Container::Horizontal(std::move(menu_buttons), &menu_selected);
    auto menu_renderer = ftxui::Renderer(menu_component, [&] {
        ftxui::Elements items;
        for (int i = 0; i < menu_entries.size(); ++i) {
            bool is_selected = (i == menu_selected);
            items.push_back(
                ftxui::text(menu_entries[i])
                | ftxui::border
                | (is_selected ? ftxui::inverted : ftxui::nothing)
                | ftxui::flex_grow
            );
        }
        return ftxui::hbox(std::move(items));
    });

    menu_renderer = menu_renderer | ftxui::CatchEvent([&](ftxui::Event event) {
        if (event == ftxui::Event::ArrowLeft) {
            menu_selected = std::max(0, menu_selected - 1);
            return true;
        }
        if (event == ftxui::Event::ArrowRight) {
            menu_selected = std::min((int)menu_entries.size() - 1, menu_selected + 1);
            return true;
        }
        if (event == ftxui::Event::Return) {
            menu_feedback = "Action: " + menu_entries[menu_selected];
            if (menu_selected == 0) { // Play/Pause
                if (Mix_PausedMusic()) Mix_ResumeMusic();
                else if (Mix_PlayingMusic()) Mix_PauseMusic();
            } else if (menu_selected == 4) { // Quit
                quit_thread = true;
                screen.Exit();
            }
            return true;
        }
        return false;
    });

    auto visualizer_renderer = ftxui::Renderer([&] {
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
            for (size_t i = 0; i < copy_n; ++i)
                mono_samples[i] = static_cast<float>(g_raw_audio_buffer[i * CHANNELS]) / 32768.0f;
            if (copy_n < mono_samples.size())
                std::fill(mono_samples.begin() + copy_n, mono_samples.end(), 0.0f);
            fft_processor.PerformFFT(mono_samples, g_fft_magnitudes);
        }

        data_generator.generate_frame(current_frame_pixels);

        ftxui::Canvas c(canvas_width, canvas_height);
        MapFrameToCanvas(c, current_frame_pixels, w, h);

        status_message = "Status: " + std::to_string(static_cast<int>(std::round(1.0f / delta_time))) + " FPS | Audio Playing: " + (Mix_PlayingMusic() ? "YES" : "NO");
        auto canvas_element = ftxui::canvas(c) | ftxui::border;
        auto status_bar = ftxui::text("(Target FPS: " + std::to_string(FPS_TARGET) + ") | " + status_message + " | Menu Feedback: " + menu_feedback) | ftxui::bold;
        return ftxui::vbox({
            status_bar,
            ftxui::separator(),
            canvas_element | ftxui::flex,
        });
    });

    auto main_container = ftxui::Container::Vertical({
        visualizer_renderer,
        menu_renderer,
    });

    auto final_renderer = ftxui::Renderer(main_container, [&] {
        return ftxui::vbox({
            visualizer_renderer->Render() | ftxui::flex,
            ftxui::separator(),
            menu_renderer->Render(),
        });
    });
    
    final_renderer = final_renderer | ftxui::CatchEvent([&](ftxui::Event event) {
        if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) {
            quit_thread = true;
            screen.Exit();
            return true;
        }
        return false;
    });

    std::thread redraw_thread_instance(RedrawThread, &screen, FPS_TARGET);
    screen.Loop(final_renderer);
    quit_thread = true;
    if (redraw_thread_instance.joinable()) redraw_thread_instance.join();
    std::cout << "Application loop finished. Total elapsed time: " << total_elapsed_time << "s\n";
}