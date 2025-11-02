// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include "image.h"

#include <cstdio>
#include <print>

#include "tracy/Tracy.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

void Image::load(const std::filesystem::path& fileName) {
    ZoneScoped;
    ZoneTextF("fileName=%s", fileName.string().c_str());
    int width, height, numChannels;
    float* buffer = stbi_loadf(
        fileName.string().c_str(), &width, &height, &numChannels, 0);
    if (!buffer) {
        std::println(stderr, "Unable to load {}", fileName.string());
        return;
    }
    _channels = numChannels;
    resize(width, height);
    std::copy(buffer, buffer + width * height * numChannels, _pixels.begin());
    stbi_image_free(buffer);
    std::println("Loaded {}", fileName.string());
}

void Image::load(const std::span<const std::byte> bytes) {
    ZoneScoped;
    ZoneTextF("bytes.size()=%zu", bytes.size());
    int width, height, numChannels;
    float* buffer = stbi_loadf_from_memory(
        reinterpret_cast<const stbi_uc*>(bytes.data()),
        static_cast<int>(bytes.size()),
        &width, &height, &numChannels, 0);
    if (!buffer) {
        std::println(stderr, "Unable to load from memory");
        return;
    }
    _channels = numChannels;
    resize(width, height);
    std::copy(buffer, buffer + width * height * numChannels, _pixels.begin());
    stbi_image_free(buffer);
    std::println("Loaded from memory");
}

void Image::save(const std::filesystem::path& fileName) const {
    ZoneScoped;
    ZoneTextF("fileName=%s", fileName.string().c_str());
    std::vector<std::uint8_t> buffer(_width * _height * _channels);
    // Vertically flip the image when saving
    for (std::size_t y = 0; y < _height; ++y) {
        std::size_t srcRow = _height - 1 - y;
        for (std::size_t x = 0; x < _width; ++x) {
            for (int c = 0; c < _channels; ++c) {
                std::size_t srcIdx = (srcRow * _width + x) * _channels + c;
                std::size_t dstIdx = (y * _width + x) * _channels + c;
                buffer[dstIdx] = static_cast<std::uint8_t>(
                    255.0f * gammaCorrection(toneMapping(_pixels[srcIdx])));
            }
        }
    }
    int stride = _width * _channels;
    int result = stbi_write_png(
        fileName.string().c_str(), _width, _height, _channels,
        buffer.data(), stride);
    if (!result) {
        std::println(stderr, "Failed to save: {}", fileName.string());
    } else {
        std::println("Saved \"{}\".", fileName.string());
    }
}
