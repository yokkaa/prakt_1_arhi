#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>
#include <filesystem>

#include "Mesh.h"
#include "MeshLoader.h"

#include "Texture.h"
#include "TextureLoader.h"

#include "ShaderProgram.h"
#include "ShaderLoader.h"

#include "RenderAdapter.h"

#include <future>
#include <thread>
#include <chrono>

#include "Material.h"
#include "MaterialLoader.h"
#include "BinaryMeshLoader.h"

class ResourceManager {
private:
    std::unordered_map<std::string, std::shared_ptr<EngineMesh>> meshes;
    std::unordered_map<std::string, std::shared_ptr<EngineTexture>> textures;
    std::unordered_map<std::string, std::shared_ptr<EngineShaderProgram>> shaders;
    std::unordered_map<std::string, std::shared_ptr<EngineMaterial>> materials;

    struct AsyncMeshEntry {
        std::future<std::shared_ptr<MeshData>> future;
        bool loading = false;
        bool ready = false;
        std::shared_ptr<EngineMesh> mesh;
    };

    std::unordered_map<std::string, AsyncMeshEntry> asyncMeshes;
    std::shared_ptr<EngineMesh> placeholderMesh;

    std::shared_ptr<MeshData> CreatePlaceholderMeshData() {
        auto data = std::make_shared<MeshData>();

        Vertex v1;
        v1.Position = glm::vec3(-0.5f, -0.5f, 0.0f);
        v1.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        v1.TexCoords = glm::vec2(0.0f, 0.0f);

        Vertex v2;
        v2.Position = glm::vec3(0.5f, -0.5f, 0.0f);
        v2.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        v2.TexCoords = glm::vec2(1.0f, 0.0f);

        Vertex v3;
        v3.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        v3.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        v3.TexCoords = glm::vec2(0.5f, 1.0f);

        data->Vertices = { v1, v2, v3 };
        data->Indices = { 0, 1, 2 };

        return data;
    }

    std::shared_ptr<EngineMesh> GetPlaceholderMesh(RenderAdapter& renderer) {
        if (placeholderMesh) {
            return placeholderMesh;
        }

        auto data = CreatePlaceholderMeshData();

        placeholderMesh = std::make_shared<EngineMesh>();
        placeholderMesh->Data = data;
        placeholderMesh->GPUHandle = renderer.UploadMesh(*data);

        return placeholderMesh;
    }

    std::string GetBinaryMeshPath(const std::string& sourcePath) {
        size_t dotPosition = sourcePath.find_last_of('.');

        if (dotPosition == std::string::npos) {
            return sourcePath + ".emesh";
        }

        return sourcePath.substr(0, dotPosition) + ".emesh";
    }

public:
    std::shared_ptr<EngineMesh> LoadMesh(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto found = meshes.find(path);
        if (found != meshes.end()) {
            return found->second;
        }

        std::string binaryPath = GetBinaryMeshPath(path);

        std::shared_ptr<MeshData> data =
            BinaryMeshLoader::LoadMesh(binaryPath);

        if (!data) {
            std::cout << "Binary mesh not found, loading source mesh: "
                << path << std::endl;

            data = MeshLoader::LoadMesh(path);

            if (!data) {
                return nullptr;
            }

            BinaryMeshLoader::SaveMesh(binaryPath, *data);
        }

        auto mesh = std::make_shared<EngineMesh>();
        mesh->Data = data;
        mesh->GPUHandle = renderer.UploadMesh(*data);

        meshes[path] = mesh;
        return mesh;
    }

    std::shared_ptr<EngineMesh> LoadMeshAsync(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto foundReady = meshes.find(path);
        if (foundReady != meshes.end()) {
            return foundReady->second;
        }

        auto foundAsync = asyncMeshes.find(path);

        if (foundAsync == asyncMeshes.end()) {
            AsyncMeshEntry entry;
            entry.loading = true;
            entry.ready = false;

            entry.future = std::async(
                std::launch::async,
                [this, path]() {
                    std::cout << "Async mesh loading started: " << path << std::endl;

                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    std::string binaryPath = GetBinaryMeshPath(path);

                    std::shared_ptr<MeshData> data =
                        BinaryMeshLoader::LoadMesh(binaryPath);

                    if (data) {
                        return data;
                    }

                    std::cout << "Binary mesh not found, loading source mesh: "
                        << path << std::endl;

                    data = MeshLoader::LoadMesh(path);

                    if (data) {
                        BinaryMeshLoader::SaveMesh(binaryPath, *data);
                    }

                    return data;
                }
            );

            asyncMeshes.emplace(path, std::move(entry));

            return GetPlaceholderMesh(renderer);
        }

        AsyncMeshEntry& entry = foundAsync->second;

        if (!entry.ready) {
            auto status = entry.future.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                std::shared_ptr<MeshData> data = entry.future.get();

                if (!data) {
                    std::cout << "Async mesh loading failed: " << path << std::endl;
                    asyncMeshes.erase(path);
                    return GetPlaceholderMesh(renderer);
                }

                auto mesh = std::make_shared<EngineMesh>();
                mesh->Data = data;

                mesh->GPUHandle = renderer.UploadMesh(*data);

                meshes[path] = mesh;

                entry.mesh = mesh;
                entry.ready = true;
                entry.loading = false;

                std::cout << "Async mesh loaded: " << path
                    << " | vertices: " << data->Vertices.size()
                    << " | indices: " << data->Indices.size()
                    << std::endl;

                return mesh;
            }

            return GetPlaceholderMesh(renderer);
        }

        if (entry.mesh) {
            return entry.mesh;
        }

        return GetPlaceholderMesh(renderer);
    }

    std::shared_ptr<EngineTexture> LoadTexture(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto found = textures.find(path);
        if (found != textures.end()) {
            return found->second;
        }

        std::shared_ptr<TextureData> data = TextureLoader::LoadTexture(path);
        if (!data) {
            return nullptr;
        }

        auto texture = std::make_shared<EngineTexture>();
        texture->Data = data;
        texture->GPUHandle = renderer.CreateTexture(*data);

        textures[path] = texture;
        return texture;
    }

    std::shared_ptr<EngineShaderProgram> LoadShader(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        RenderAdapter& renderer
    ) {
        std::string key = vertexPath + "|" + fragmentPath;

        auto found = shaders.find(key);
        if (found != shaders.end()) {
            return found->second;
        }

        std::shared_ptr<ShaderSource> source = ShaderLoader::LoadShaderSource(
            vertexPath,
            fragmentPath
        );

        if (!source) {
            return nullptr;
        }

        auto shader = std::make_shared<EngineShaderProgram>();
        shader->Source = source;
        shader->GPUHandle = renderer.CreateShaderProgram(
            source->VertexSource,
            source->FragmentSource
        );

        if (shader->GPUHandle.ID == 0) {
            return nullptr;
        }

        shader->VertexPath = vertexPath;
        shader->FragmentPath = fragmentPath;
        shader->VertexLastWriteTime = std::filesystem::last_write_time(vertexPath);
        shader->FragmentLastWriteTime = std::filesystem::last_write_time(fragmentPath);

        shaders[key] = shader;
        return shader;
    }

    std::shared_ptr<EngineMaterial> LoadMaterial(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto found = materials.find(path);
        if (found != materials.end()) {
            return found->second;
        }

        std::shared_ptr<MaterialData> data =
            MaterialLoader::LoadMaterial(path);

        if (!data) {
            return nullptr;
        }

        auto material = std::make_shared<EngineMaterial>();
        material->Data = data;

        if (!data->texturePath.empty()) {
            material->Texture = LoadTexture(data->texturePath, renderer);
        }

        if (!data->vertexShaderPath.empty() &&
            !data->fragmentShaderPath.empty()) {
            material->Shader = LoadShader(
                data->vertexShaderPath,
                data->fragmentShaderPath,
                renderer
            );
        }

        materials[path] = material;

        std::cout << "Material loaded: " << path << std::endl;

        return material;
    }

    void ReloadChangedShaders(RenderAdapter& renderer) {
        for (auto& pair : shaders) {
            auto& shader = pair.second;

            if (!shader) {
                continue;
            }

            if (!std::filesystem::exists(shader->VertexPath) ||
                !std::filesystem::exists(shader->FragmentPath)) {
                continue;
            }

            auto currentVertexTime =
                std::filesystem::last_write_time(shader->VertexPath);

            auto currentFragmentTime =
                std::filesystem::last_write_time(shader->FragmentPath);

            bool changed =
                currentVertexTime != shader->VertexLastWriteTime ||
                currentFragmentTime != shader->FragmentLastWriteTime;

            if (!changed) {
                continue;
            }

            std::cout << "Shader changed, reloading: "
                << shader->VertexPath << " | "
                << shader->FragmentPath << std::endl;

            std::shared_ptr<ShaderSource> newSource =
                ShaderLoader::LoadShaderSource(
                    shader->VertexPath,
                    shader->FragmentPath
                );

            if (!newSource) {
                std::cout << "Shader reload failed: source loading error" << std::endl;
                continue;
            }

            GPUShaderProgram newProgram = renderer.CreateShaderProgram(
                newSource->VertexSource,
                newSource->FragmentSource
            );

            if (newProgram.ID == 0) {
                std::cout << "Shader reload failed: compilation error" << std::endl;
                continue;
            }

            renderer.DeleteShaderProgram(shader->GPUHandle);

            shader->Source = newSource;
            shader->GPUHandle = newProgram;
            shader->VertexLastWriteTime = currentVertexTime;
            shader->FragmentLastWriteTime = currentFragmentTime;

            std::cout << "Shader reloaded successfully" << std::endl;
        }
    }
};