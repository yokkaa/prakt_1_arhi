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

#include "JobSystem.h"
#include <deque>
#include <unordered_set>

class ResourceManager {
private:
    JobSystem* jobSystem = nullptr;
    std::deque<std::string> pendingMeshUploads;
    std::unordered_set<std::string> failedMeshes;
        std::unordered_map<
        std::string,
        std::future<std::shared_ptr<TextureData>>
    > asyncTextures;

    std::deque<std::string> pendingTextureUploads;
    std::shared_ptr<EngineTexture> placeholderTexture;  

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
        std::shared_ptr<EngineTexture> GetPlaceholderTexture(
        RenderAdapter& renderer
    ) {
        if (placeholderTexture) {
            return placeholderTexture;
        }

        auto data = std::make_shared<TextureData>();
        data->Width = 2;
        data->Height = 2;
        data->Channels = 4;

        // Два серых оттенка, четыре пикселя RGBA.
        data->Pixels = {
            200, 200, 200, 255,
             70,  70,  70, 255,
             70,  70,  70, 255,
            200, 200, 200, 255
        };

        placeholderTexture = std::make_shared<EngineTexture>();
        placeholderTexture->Data = data;
        placeholderTexture->GPUHandle =
            renderer.CreateTexture(*data);

        return placeholderTexture;
    }

public:
    void SetJobSystem(JobSystem& system) {
        jobSystem = &system;
    }
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
        auto ready = meshes.find(path);

        if (ready != meshes.end()) {
            return ready->second;
        }

        if (failedMeshes.count(path) != 0) {
            return GetPlaceholderMesh(renderer);
        }

        if (asyncMeshes.find(path) == asyncMeshes.end()) {
            if (!jobSystem) {
                throw std::runtime_error(
                    "ResourceManager: JobSystem is not connected"
                );
            }

            const std::string binaryPath = GetBinaryMeshPath(path);

            AsyncMeshEntry entry;
            entry.loading = true;

            entry.future = jobSystem->Submit(
                [path, binaryPath]() -> std::shared_ptr<MeshData> {
                    auto data = BinaryMeshLoader::LoadMesh(binaryPath);

                    if (!data) {
                        data = MeshLoader::LoadMesh(path);
                    }

                    return data;
                }
            );

            asyncMeshes.emplace(path, std::move(entry));
            pendingMeshUploads.push_back(path);

            std::cout << "Mesh queued in JobSystem: "
                      << path << std::endl;
        }

        return GetPlaceholderMesh(renderer);
    }

        void PumpMeshUploads(
        RenderAdapter& renderer,
        std::size_t maxUploads = 2,
        double budgetMilliseconds = 2.0
    ) {
        if (maxUploads == 0 || budgetMilliseconds <= 0.0) {
            return;
        }

        using Clock = std::chrono::steady_clock;

        const auto started = Clock::now();

        // За один вызов проверяем каждый текущий элемент
        // очереди не более одного раза.
        std::size_t checksRemaining = pendingMeshUploads.size();
        std::size_t uploaded = 0;

        while (checksRemaining > 0 &&
               !pendingMeshUploads.empty() &&
               uploaded < maxUploads) {

            const double elapsedMilliseconds =
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();

            if (elapsedMilliseconds >= budgetMilliseconds) {
                break;
            }

            --checksRemaining;

            std::string path =
                std::move(pendingMeshUploads.front());

            pendingMeshUploads.pop_front();

            auto entry = asyncMeshes.find(path);

            if (entry == asyncMeshes.end()) {
                continue;
            }

            const auto status = entry->second.future.wait_for(
                std::chrono::seconds(0)
            );

            if (status != std::future_status::ready) {
                pendingMeshUploads.push_back(std::move(path));
                continue;
            }

            std::shared_ptr<MeshData> data;

            try {
                data = entry->second.future.get();
            }
            catch (const std::exception& error) {
                std::cerr << "Mesh job failed: "
                          << path << " | "
                          << error.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Mesh job failed: "
                          << path << " | unknown exception"
                          << std::endl;
            }

            asyncMeshes.erase(entry);

            if (!data) {
                failedMeshes.insert(path);

                std::cerr << "Mesh unavailable: "
                          << path << std::endl;

                continue;
            }

            auto mesh = std::make_shared<EngineMesh>();
            mesh->Data = data;

            // Эта функция вызывается только главным потоком.
            mesh->GPUHandle = renderer.UploadMesh(*data);

            meshes[path] = mesh;
            ++uploaded;

            std::cout << "Mesh uploaded on main thread: "
                      << path << std::endl;
        }
    }

        std::shared_ptr<EngineTexture> LoadTexture(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto found = textures.find(path);

        if (found != textures.end()) {
            return found->second;
        }

        if (!jobSystem) {
            throw std::runtime_error(
                "ResourceManager: JobSystem is not connected"
            );
        }

        auto placeholder = GetPlaceholderTexture(renderer);

        // Отдельный объект для каждого пути.
        // Пока все используют одну GPU-текстуру заглушки.
        auto texture = std::make_shared<EngineTexture>();
        texture->Data = placeholder->Data;
        texture->GPUHandle = placeholder->GPUHandle;

        auto future = jobSystem->Submit(
            [path]() -> std::shared_ptr<TextureData> {
                return TextureLoader::LoadTexture(path);
            }
        );

        textures.emplace(path, texture);
        asyncTextures.emplace(path, std::move(future));
        pendingTextureUploads.push_back(path);

        std::cout << "Texture queued in JobSystem: "
                  << path << std::endl;

        return texture;
    }

    void PumpTextureUploads(
        RenderAdapter& renderer,
        std::size_t maxUploads = 2,
        double budgetMilliseconds = 2.0
    ) {
        if (maxUploads == 0 || budgetMilliseconds <= 0.0) {
            return;
        }

        using Clock = std::chrono::steady_clock;

        const auto started = Clock::now();
        std::size_t checksRemaining = pendingTextureUploads.size();
        std::size_t uploaded = 0;

        while (checksRemaining > 0 &&
               !pendingTextureUploads.empty() &&
               uploaded < maxUploads) {

            const double elapsedMilliseconds =
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();

            if (elapsedMilliseconds >= budgetMilliseconds) {
                break;
            }

            --checksRemaining;

            std::string path =
                std::move(pendingTextureUploads.front());

            pendingTextureUploads.pop_front();

            auto pending = asyncTextures.find(path);

            if (pending == asyncTextures.end()) {
                continue;
            }

            const auto status = pending->second.wait_for(
                std::chrono::seconds(0)
            );

            if (status != std::future_status::ready) {
                pendingTextureUploads.push_back(std::move(path));
                continue;
            }

            std::shared_ptr<TextureData> data;

            try {
                data = pending->second.get();
            }
            catch (const std::exception& error) {
                std::cerr << "Texture job failed: "
                          << path << " | "
                          << error.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Texture job failed: "
                          << path << " | unknown exception"
                          << std::endl;
            }

            asyncTextures.erase(pending);

            if (!data) {
                // Объект остаётся с заглушкой.
                // Повторные запросы не запускают загрузку заново.
                std::cerr << "Texture unavailable: "
                          << path << std::endl;
                continue;
            }

            auto target = textures.find(path);

            if (target == textures.end()) {
                continue;
            }

            const GPUTexture gpuTexture =
                renderer.CreateTexture(*data);

            // Считаем и неудачную попытку создания:
            // она тоже потратила время главного потока.
            ++uploaded;

            if (gpuTexture.ID == 0) {
                std::cerr << "Texture GPU upload failed: "
                          << path << std::endl;
                continue;
            }

            // Обновляем объект, на который уже ссылается материал.
            target->second->Data = std::move(data);
            target->second->GPUHandle = gpuTexture;

            std::cout << "Texture uploaded on main thread: "
                      << path << std::endl;
        }
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