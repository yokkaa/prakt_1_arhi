#pragma once
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

class MeshLoader {
public:
    static std::shared_ptr<MeshData> LoadMesh(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || !scene->HasMeshes()) {
            std::cerr << "Failed to load mesh: " << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        // Берём первый меш в сцене
        aiMesh* mesh = scene->mMeshes[0];
        auto meshData = std::make_shared<MeshData>();

        // Копируем вершины
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.Normal = mesh->HasNormals() ?
                glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
            vertex.TexCoords = mesh->HasTextureCoords(0) ?
                glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
            meshData->Vertices.push_back(vertex);
        }

        // Копируем индексы
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                meshData->Indices.push_back(face.mIndices[j]);
            }
        }

        return meshData;
    }
};
