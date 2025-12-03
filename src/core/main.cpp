#include <iostream>
#include <string>

void RunAppLoop(const float FPS_TARGET); 

bool InitializeAudio();
bool StartAudioPlayback(const std::string& file_path, int loops = 0);
void CleanupAudio();

const float FPS_TARGET = 30.0f; 

int main(int argc, char* argv[]) {
    std::cout << "Starting Audio Visualizer Simulation..." << std::endl;

    InitializeAudio();
    StartAudioPlayback("/home/ezroot/Repos/cmusic/build/godisaweapon.mp3");
    RunAppLoop(FPS_TARGET);
    CleanupAudio();
    return 0;
}