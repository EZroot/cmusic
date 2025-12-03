#pragma once

#include <chrono>
#include <atomic>
#include <ftxui/component/screen_interactive.hpp>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<float>;

extern std::atomic_bool quit_thread; 

void RedrawThread(ftxui::ScreenInteractive* screen_ptr, float target_fps);