#include "BinaryMeshLoader.h"

#include <fstream>
#include <iostream>
#include <cstdint>

struct BinaryMeshHeader {
    char magic[4];          // EMSH
    uint32_t version;
    uint32_t vertexCount;
    uint32_t indexCount;
};

bool BinaryMeshLoader::SaveMesh(
    const std::string& path,
    const MeshData& meshData
) {
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to save binary mesh: " << path << std::endl;
        return false;
    }

    BinaryMeshHeader header;
    header.magic[0] = 'E';
    header.magic[1] = 'M';
    header.magic[2] = 'S';
    header.magic[3] = 'H';
    header.version = 1;
    header.vertexCount = static_cast<uint32_t>(meshData.Vertices.size());
    header.indexCount = static_cast<uint32_t>(meshData.Indices.size());

    file.write(
        reinterpret_cast<const char*>(&header),
        sizeof(BinaryMeshHeader)
    );

    file.write(
        reinterpret_cast<const char*>(meshData.Vertices.data()),
        meshData.Vertices.size() * sizeof(Vertex)
    );

    file.write(
        reinterpret_cast<const char*>(meshData.Indices.data()),
        meshData.Indices.size() * sizeof(unsigned int)
    );

    std::cout << "Binary mesh saved: " << path
        << " | vertices: " << meshData.Vertices.size()
        << " | indices: " << meshData.Indices.size()
        << std::endl;

    return true;
}

std::shared_ptr<MeshData> BinaryMeshLoader::LoadMesh(
    const std::string& path
) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        return nullptr;
    }

    BinaryMeshHeader header;
    file.read(
        reinterpret_cast<char*>(&header),
        sizeof(BinaryMeshHeader)
    );

    if (header.magic[0] != 'E' ||
        header.magic[1] != 'M' ||
        header.magic[2] != 'S' ||
        header.magic[3] != 'H') {
        std::cerr << "Invalid binary mesh file: " << path << std::endl;
        return nullptr;
    }

    if (header.version != 1) {
        std::cerr << "Unsupported binary mesh version: " << header.version << std::endl;
        return nullptr;
    }

    auto meshData = std::make_shared<MeshData>();

    meshData->Vertices.resize(header.vertexCount);
    meshData->Indices.resize(header.indexCount);

    file.read(
        reinterpret_cast<char*>(meshData->Vertices.data()),
        meshData->Vertices.size() * sizeof(Vertex)
    );

    file.read(
        reinterpret_cast<char*>(meshData->Indices.data()),
        meshData->Indices.size() * sizeof(unsigned int)
    );

    if (!file.good()) {
        std::cerr << "Failed to read binary mesh data: " << path << std::endl;
        return nullptr;
    }

    std::cout << "Binary mesh loaded: " << path
        << " | vertices: " << meshData->Vertices.size()
        << " | indices: " << meshData->Indices.size()
        << std::endl;

    return meshData;
}