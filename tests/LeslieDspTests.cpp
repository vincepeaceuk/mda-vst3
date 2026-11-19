#include "../shared/LeslieDsp.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try {
        for (double rate : {44100.0, 48000.0, 96000.0, 192000.0}) {
            mda::LeslieDsp dsp;
            dsp.prepare(rate);
            constexpr int block = 257;
            std::array<float, block> left{}, right{};
            std::array<float, 9> params{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f};
            dsp.setParameters(params);
            double energy = 0.0;
            for (int n = 0; n < 300; ++n) {
                for (int i = 0; i < block; ++i)
                    left[i] = right[i] = 0.1f * std::sin(0.05f * (n * block + i));
                dsp.process(left.data(), right.data(), block);
                for (int i = 0; i < block; ++i) {
                    require(std::isfinite(left[i]) && std::isfinite(right[i]), "non-finite Leslie audio");
                    require(std::abs(left[i]) < 10.0f && std::abs(right[i]) < 10.0f, "unbounded Leslie audio");
                    energy += left[i] * left[i] + right[i] * right[i];
                }
            }
            require(energy > 0.1, "Leslie is silent");
            dsp.reset();
            left.fill(0.0f); right.fill(0.0f);
            dsp.process(left.data(), right.data(), block);
            for (auto v : left) require(v == 0.0f, "Leslie reset leaves audio in delay");
            std::cout << "PASS maximum depth/speed and clean reset at " << rate << " Hz\n";
        }
        mda::LeslieDsp dsp;
        dsp.prepare(44100.0);
        std::array<float, 9> params{0.5f, .5f, .5f, .7f, .6f, .7f, .48f, .5f, .6f};
        dsp.setParameters(params);
        std::array<float, 512> left{}, right{};
        left[0] = right[0] = 0.5f;
        dsp.process(left.data(), right.data(), 512);
        const auto reference = left;
        dsp.reset();
        left.fill(0.0f); right.fill(0.0f);
        left[0] = right[0] = 0.5f;
        dsp.process(left.data(), right.data(), 512);
        require(left == reference, "Leslie reset is not deterministic");
        std::cout << "PASS deterministic original-default impulse response\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
