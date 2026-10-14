#pragma once
#include <vector>
#include <glm/glm.hpp> // ��� vec3, vec2
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
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    size_t IndexCount = 0;
};

struct EngineMesh {
    std::shared_ptr<MeshData> Data;
    GPUMesh GPUHandle;
};
