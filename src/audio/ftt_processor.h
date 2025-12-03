#pragma once
#include "constants.h"
#include <vector>
#include <kissfft/kiss_fftr.h>
#include <complex>
class SimpleFFT {
public:
    SimpleFFT();
    ~SimpleFFT();
    void PerformFFT(const std::vector<float>& mono_samples, std::vector<float>& magnitude_bands);
private:
    kiss_fftr_cfg cfg_ = nullptr;
    kiss_fft_cpx* c_out_ = nullptr;
    std::vector<float> r_in_;
    int output_size_ = 0;
};