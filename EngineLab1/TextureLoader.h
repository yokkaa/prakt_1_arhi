#pragma once

#include <memory>
#include <string>

#include "Texture.h"

class TextureLoader {
public:
    static std::shared_ptr<TextureData> LoadTexture(const std::string& path);
};