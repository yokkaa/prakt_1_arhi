#pragma once

#include <memory>
#include <string>

#include "Mesh.h"

class BinaryMeshLoader {
public:
    static bool SaveMesh(
        const std::string& path,
        const MeshData& meshData
    );

    static std::shared_ptr<MeshData> LoadMesh(
        const std::string& path
    );
};
