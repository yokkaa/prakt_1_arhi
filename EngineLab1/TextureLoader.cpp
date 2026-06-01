#include "TextureLoader.h"

#include <iostream>

#include "stb_image.h"

std::shared_ptr<TextureData> TextureLoader::LoadTexture(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        0
    );

    if (!pixels) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "stb_image error: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    auto textureData = std::make_shared<TextureData>();
    textureData->Width = width;
    textureData->Height = height;
    textureData->Channels = channels;

    size_t dataSize = static_cast<size_t>(width) * height * channels;
    textureData->Pixels.assign(pixels, pixels + dataSize);

    stbi_image_free(pixels);

    return textureData;
}