#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "json.hpp"
#include <iostream>

int main() {
    // --- Тест JSON ---
    nlohmann::json j;
    j["test"] = 123;
    std::cout << "JSON test: " << j.dump() << std::endl;

    // --- Тест загрузки модели через Assimp ---
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("backpack.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return -1;
    }
    std::cout << "Model loaded successfully! Mesh count: " << scene->mNumMeshes << std::endl;

    // --- Тест загрузки текстуры через stb_image ---
    int w, h, ch;
    unsigned char* data = stbi_load("texture.png", &w, &h, &ch, 0);
    if (!data) std::cerr << "Failed to load texture\n";
    else {
        std::cout << "Texture loaded: " << w << "x" << h << " channels: " << ch << std::endl;
        stbi_image_free(data);
    }

    return 0;
}