#pragma once

#include <string>
#include <memory>

#include "Components.h"
#include "Texture.h"
#include "ShaderProgram.h"

struct MaterialData {
    Color4 baseColor{ 255, 255, 255, 255 };

    std::string texturePath;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
};

struct EngineMaterial {
    std::shared_ptr<MaterialData> Data;

    std::shared_ptr<EngineTexture> Texture;
    std::shared_ptr<EngineShaderProgram> Shader;
};
