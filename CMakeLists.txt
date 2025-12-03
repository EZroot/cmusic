cmake_minimum_required(VERSION 3.14)

project(KubuntuTUIProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------
# SDL2
# --------------------------
find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_mixer QUIET CONFIG)

if(NOT SDL2_mixer_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_mixer REQUIRED sdl2-mixer)

    add_library(SDL2_mixer::SDL2_mixer INTERFACE IMPORTED)
    target_include_directories(SDL2_mixer::SDL2_mixer INTERFACE ${SDL2_mixer_INCLUDE_DIRS})
    target_link_libraries(SDL2_mixer::SDL2_mixer INTERFACE ${SDL2_mixer_LIBRARIES})
endif()

# --------------------------
# KissFFT (system package)
# --------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(KISSFFT REQUIRED kissfft-float)

add_library(KissFFT::kissfft INTERFACE IMPORTED)
target_include_directories(KissFFT::kissfft INTERFACE ${KISSFFT_INCLUDE_DIRS})
target_link_libraries(KissFFT::kissfft INTERFACE ${KISSFFT_LIBRARIES})

# --------------------------
# FTXUI
# --------------------------
include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v4.0.0
)

FetchContent_MakeAvailable(ftxui)

# --------------------------
# Executable
# --------------------------
add_executable(cmusic
    src/core/main.cpp
    src/rendering/screen_loop.cpp
    src/audio/audio_pipe.cpp
)

target_link_libraries(cmusic PRIVATE
    ftxui::dom
    ftxui::screen
    ftxui::component
    SDL2::SDL2
    SDL2_mixer::SDL2_mixer
    KissFFT::kissfft
)

target_include_directories(cmusic PRIVATE
    src/media
    src/rendering
    src/audio
)
