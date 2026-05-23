#pragma once

#include <vector>
#include <memory>

struct TextureData {
    int Width = 0;
    int Height = 0;
    int Channels = 0;

    std::vector<unsigned char> Pixels;
};

struct GPUTexture {
    unsigned int ID = 0;

    int Width = 0;
    int Height = 0;
    int Channels = 0;
};

struct EngineTexture {
    std::shared_ptr<TextureData> Data;
    GPUTexture GPUHandle;
};
