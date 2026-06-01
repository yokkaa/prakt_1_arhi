#include "MaterialLoader.h"

#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::shared_ptr<MaterialData> MaterialLoader::LoadMaterial(
    const std::string& path
) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to open material file: " << path << std::endl;
        return nullptr;
    }

    json materialJson;
    file >> materialJson;

    auto material = std::make_shared<MaterialData>();

    if (materialJson.contains("baseColor")) {
        const auto& color = materialJson["baseColor"];

        material->baseColor.r = color.value("r", 255);
        material->baseColor.g = color.value("g", 255);
        material->baseColor.b = color.value("b", 255);
        material->baseColor.a = color.value("a", 255);
    }

    material->texturePath =
        materialJson.value("texture", "");

    material->vertexShaderPath =
        materialJson.value("vertexShader", "");

    material->fragmentShaderPath =
        materialJson.value("fragmentShader", "");

    return material;
}