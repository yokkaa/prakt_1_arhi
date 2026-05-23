#pragma once
#include <vector>
#include <glm/glm.hpp> // для vec3, vec2
#include <memory>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct MeshData {
    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;
};

struct GPUMesh {
    unsigned int VAO;       // Vertex Array Object
    unsigned int VBO;       // Vertex Buffer Object
    unsigned int EBO;       // Element Buffer Object
    size_t IndexCount;      // Количество индексов
};

struct EngineMesh {
    std::shared_ptr<MeshData> Data;
    GPUMesh GPUHandle;
};
