#pragma once

#include <string>
#include <memory>
#include <filesystem>

struct ShaderSource {
    std::string VertexSource;
    std::string FragmentSource;
};

struct GPUShaderProgram {
    unsigned int ID = 0;
};

struct EngineShaderProgram {
    std::shared_ptr<ShaderSource> Source;
    GPUShaderProgram GPUHandle;

    std::string VertexPath;
    std::string FragmentPath;

    std::filesystem::file_time_type VertexLastWriteTime;
    std::filesystem::file_time_type FragmentLastWriteTime;
};
