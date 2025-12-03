#pragma once
#include "constants.h"
#include <vector>
#include <ftxui/dom/canvas.hpp>

class AudioDataGenerator {
public:
    AudioDataGenerator();
    void generate_frame(std::vector<unsigned char>& data);
    int width() const { return MIRRORED_WIDTH; }
    int height() const { return VISUALIZER_HEIGHT; }
    int canvas_width() const { return width(); }
    int canvas_height() const { return height(); }
};

void MapFrameToCanvas(ftxui::Canvas& c, const std::vector<unsigned char>& data, int data_w, int data_h);