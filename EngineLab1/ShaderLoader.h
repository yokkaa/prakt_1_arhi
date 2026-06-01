#pragma once

#include <memory>
#include <string>

#include "ShaderProgram.h"

class ShaderLoader {
public:
    static std::shared_ptr<ShaderSource> LoadShaderSource(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );
};