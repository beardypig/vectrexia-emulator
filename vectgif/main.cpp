#include <cstdlib>
#include <memory>
#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <string_view>
#include <optional>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <vectrexia.h>
#include "gif.h"
#include <cxxopts.hpp>

constexpr size_t ROM_SIZE = 65536;
constexpr size_t MAX_FILENAME_SIZE = 2000;

std::unique_ptr<Vectrex> vectrex = std::make_unique<Vectrex>();
std::array<uint8_t, FRAME_WIDTH * FRAME_HEIGHT * 4> gif_buffer{};

int main(int argc, char *argv[])
{
    long skipframes = 0;
    long outframes = 1000;
    std::array<uint8_t, ROM_SIZE> rombuffer{};
    GifWriter gw{};

    // Parse command line arguments using cxxopts
    cxxopts::Options options("vectgif", "Generate a GIF from a Vectrex ROM");
    options.add_options()
        ("s,skipframes", "Number of frames to skip", cxxopts::value<long>()->default_value("0"))
        ("n,outframes", "Number of output frames", cxxopts::value<long>()->default_value("1000"))
        ("rom", "ROM file", cxxopts::value<std::string>())
        ("gif", "GIF output file", cxxopts::value<std::string>()->default_value(""));

    auto result = options.parse(argc, argv);

    skipframes = result["skipframes"].as<long>();
    outframes = result["outframes"].as<long>();

    if (!result.count("rom")) {
        std::cerr << "vectgif: usage: vectgif <rom> [gif]\n";
        return 1;
    }

    std::string rom_filename = result["rom"].as<std::string>();
    std::string giffilename;
    if (result.count("gif") && !result["gif"].as<std::string>().empty()) {
        giffilename = result["gif"].as<std::string>();
    } else {
        giffilename = fmt::format("{}.gif", rom_filename);
    }

    // Load the ROM file
    std::ifstream romfile(rom_filename, std::ios::binary);
    if (!romfile) {
        std::cerr << fmt::format("[ROM]: Failed to open ROM file {}\n", rom_filename);
        return 1;
    }

    std::cout << fmt::format("[ROM]: loading ROM \"{}\"\n", rom_filename);
    romfile.read(reinterpret_cast<char*>(rombuffer.data()), ROM_SIZE);
    auto r = static_cast<size_t>(romfile.gcount());
    romfile.close();

    std::cout << fmt::format("[ROM]: size = {}\n", r);

    if (!vectrex->LoadCartridge(rombuffer.data(), r)) {
        std::cerr << "Failed to load the ROM file\n";
        return 1;
    }

    GifBegin(&gw, giffilename.c_str(), FRAME_WIDTH, FRAME_HEIGHT, 2, 8, false);

    vectrex->Reset();

    vectrex->SetPlayerOne(0x80, 0x80, 1, 1, 1, 1);
    vectrex->SetPlayerTwo(0x80, 0x80, 1, 1, 1, 1);

    for (int frame = 0; frame < outframes; frame++) {
        for (int s = 0; s < skipframes + 1; s++) {
            vectrex->Run(30000);
        }

        auto framebuffer = vectrex->getFramebuffer();

        auto gb = gif_buffer.begin();
        for (const auto &fb : *framebuffer) {
            *gb++ = static_cast<uint8_t>(fb.value * 0xffu);
            *gb++ = static_cast<uint8_t>(fb.value * 0xffu);
            *gb++ = static_cast<uint8_t>(fb.value * 0xffu);
            *gb++ = static_cast<uint8_t>(fb.value * 0xffu);
        }
        GifWriteFrame(&gw, gif_buffer.data(), FRAME_WIDTH, FRAME_HEIGHT, 2);
        if (frame % 100 == 0) {
            std::cout << fmt::format("[VECTREX] frame = {}\n", frame);
        }
    }

    GifEnd(&gw);

    return 0;
}
