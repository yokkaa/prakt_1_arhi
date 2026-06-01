#include "ShaderLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>

static std::string ReadTextFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::shared_ptr<ShaderSource> ShaderLoader::LoadShaderSource(
    const std::string& vertexPath,
    const std::string& fragmentPath
) {
    std::string vertexSource = ReadTextFile(vertexPath);
    std::string fragmentSource = ReadTextFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to load shader sources." << std::endl;
        return nullptr;
    }

    auto source = std::make_shared<ShaderSource>();
    source->VertexSource = vertexSource;
    source->FragmentSource = fragmentSource;

    return source;
}