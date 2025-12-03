#include "constants.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <algorithm>

static Mix_Music* g_music = nullptr;

void SDLCALL PostMixCallback(void* userdata, Uint8* stream, int len) {
    if (len != AUDIO_BUFFER_SAMPLES * CHANNELS * sizeof(int16_t)) return;

    const int16_t* source_samples = reinterpret_cast<const int16_t*>(stream);
    std::copy(source_samples, source_samples + (len / sizeof(int16_t)), g_raw_audio_buffer.begin());
    g_new_audio_data_ready = true;
}

bool InitializeAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int initialized = Mix_Init(flags);
    
    if ((initialized & flags) != flags) {
        std::cerr << "SDL_mixer could not initialize required formats! Mix_Init Error: " << Mix_GetError() << std::endl;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, AUDIO_BUFFER_SAMPLES) < 0) {
        std::cerr << "SDL_mixer could not open audio! Mix_Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    Mix_SetPostMix(PostMixCallback, nullptr);

    return true;
}

bool StartAudioPlayback(const std::string& file_path, int loops = -1) {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
    if (g_music) {
        Mix_FreeMusic(g_music);
        g_music = nullptr;
    }

    g_music = Mix_LoadMUS(file_path.c_str()); 
    if (!g_music) {
        std::cerr << "Failed to load music file: " << file_path << "! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    if (Mix_PlayMusic(g_music, loops) == -1) { 
        std::cerr << "Failed to play music! SDL_mixer Error: " << Mix_GetError() << std::endl;
        Mix_FreeMusic(g_music);
        g_music = nullptr;
        return false;
    }

    std::cout << "Successfully loaded and started playing: " << file_path << std::endl;
    return true;
}

void CleanupAudio() {
    Mix_SetPostMix(nullptr, nullptr); 
    
    if (g_music) {
        Mix_FreeMusic(g_music);
        g_music = nullptr;
    }
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    std::cout << "Audio system cleaned up." << std::endl;
}