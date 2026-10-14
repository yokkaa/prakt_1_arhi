#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <exception>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <type_traits>

#include "Mesh.h"
#include "MeshLoader.h"
#include "BinaryMeshLoader.h"
#include "Texture.h"
#include "TextureLoader.h"
#include "ShaderProgram.h"
#include "ShaderLoader.h"
#include "Material.h"
#include "MaterialLoader.h"
#include "RenderAdapter.h"
#include "JobSystem.h"
#include <tracy/Tracy.hpp>

class ResourceManager {
private:
    JobSystem* jobSystem = nullptr;
    bool asynchronousLoading = true;

template <typename Function>
auto SubmitResourceTask(Function&& function)
    -> std::future<std::invoke_result_t<std::decay_t<Function>&>>
{
    using Result =
        std::invoke_result_t<std::decay_t<Function>&>;

    if (asynchronousLoading) {
        if (!jobSystem) {
            throw std::runtime_error(
                "ResourceManager: JobSystem is not connected"
            );
        }

        return jobSystem->Submit(
            std::forward<Function>(function)
        );
    }

    // Режим сравнения: та же работа на главном потоке.
    // packaged_task сохраняет результат или исключение в future.
    ZoneScopedN("Resources::SynchronousCPU");

    std::packaged_task<Result()> task(
        std::forward<Function>(function)
    );

    auto result = task.get_future();
    task();

    return result;
}

    struct PreparedMaterial {
        std::shared_ptr<MaterialData> data;
        std::shared_ptr<ShaderSource> shaderSource;

        std::filesystem::file_time_type vertexTime{};
        std::filesystem::file_time_type fragmentTime{};
    };

    std::unordered_map<
        std::string,
        std::future<PreparedMaterial>
    > asyncMaterials;

    std::deque<std::string> pendingMaterials;

    std::size_t uploadTurn = 0;

    std::unordered_map<
        std::string,
        std::shared_ptr<EngineMesh>
    > meshes;

    std::unordered_map<
        std::string,
        std::shared_ptr<EngineTexture>
    > textures;

    std::unordered_map<
        std::string,
        std::shared_ptr<EngineShaderProgram>
    > shaders;

    std::unordered_map<
        std::string,
        std::shared_ptr<EngineMaterial>
    > materials;

    struct AsyncMeshEntry {
        std::future<std::shared_ptr<MeshData>> future;
    };

    std::unordered_map<std::string, AsyncMeshEntry> asyncMeshes;
    std::deque<std::string> pendingMeshUploads;
    std::unordered_set<std::string> failedMeshes;
    std::shared_ptr<EngineMesh> placeholderMesh;

    std::unordered_map<
        std::string,
        std::future<std::shared_ptr<TextureData>>
    > asyncTextures;

    std::deque<std::string> pendingTextureUploads;
    std::shared_ptr<EngineTexture> placeholderTexture;

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

    std::shared_ptr<EngineMesh> GetPlaceholderMesh(
        RenderAdapter& renderer
    ) {
        if (placeholderMesh) {
            return placeholderMesh;
        }

        auto data = CreatePlaceholderMeshData();

        placeholderMesh = std::make_shared<EngineMesh>();
        placeholderMesh->Data = data;
        placeholderMesh->GPUHandle = renderer.UploadMesh(*data);

        return placeholderMesh;
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

        data->Pixels = {
            200, 200, 200, 255,
             70,  70,  70, 255,
             70,  70,  70, 255,
            200, 200, 200, 255
        };

        placeholderTexture = std::make_shared<EngineTexture>();
        placeholderTexture->Data = data;
        placeholderTexture->GPUHandle = renderer.CreateTexture(*data);

        return placeholderTexture;
    }

    std::string GetBinaryMeshPath(
        const std::string& sourcePath
    ) {
        const std::size_t dotPosition =
            sourcePath.find_last_of('.');

        if (dotPosition == std::string::npos) {
            return sourcePath + ".emesh";
        }

        return sourcePath.substr(0, dotPosition) + ".emesh";
    }

public:
    // Вызывать из главного потока.
    // Учитывает фоновые задачи и результаты,
    // которые ещё ожидают финализации.
    bool HasPendingLoads() const {
        return
            !asyncMeshes.empty() ||
            !asyncTextures.empty() ||
            !asyncMaterials.empty();
    }

    void SetJobSystem(JobSystem& system) {
        jobSystem = &system;
    }

    // Менять режим только после ReleaseAll(), до новой загрузки.
    void SetAsynchronousLoading(bool enabled) {
    asynchronousLoading = enabled;
    }

    // Вызывать из главного потока до закрытия
    // графического контекста.
    void ReleaseAll(RenderAdapter& renderer) {
        // Фоновые задачи захватывают копии путей
        // и не обращаются к этому ResourceManager.
        pendingMeshUploads.clear();
        pendingTextureUploads.clear();
        pendingMaterials.clear();

        asyncMeshes.clear();
        asyncTextures.clear();
        asyncMaterials.clear();

        materials.clear();

        for (auto& pair : meshes) {
            auto& mesh = pair.second;

            if (mesh) {
                renderer.DeleteMesh(mesh->GPUHandle);
                mesh->GPUHandle = {};
            }
        }

        meshes.clear();

        if (placeholderMesh) {
            renderer.DeleteMesh(placeholderMesh->GPUHandle);
            placeholderMesh->GPUHandle = {};
            placeholderMesh.reset();
        }

        const unsigned int placeholderId = placeholderTexture
            ? placeholderTexture->GPUHandle.ID
            : 0;

        for (auto& pair : textures) {
            auto& texture = pair.second;

            if (!texture) {
                continue;
            }

            // Общую текстуру заглушки удалим отдельно.
            if (texture->GPUHandle.ID != placeholderId) {
                renderer.DeleteTexture(texture->GPUHandle);
            }

            texture->GPUHandle = {};
        }

        textures.clear();

        if (placeholderTexture) {
            renderer.DeleteTexture(placeholderTexture->GPUHandle);
            placeholderTexture->GPUHandle = {};
            placeholderTexture.reset();
        }

        for (auto& pair : shaders) {
            auto& shader = pair.second;

            if (shader) {
                renderer.DeleteShaderProgram(shader->GPUHandle);
                shader->GPUHandle = {};
            }
        }

        shaders.clear();
        failedMeshes.clear();

        jobSystem = nullptr;
        uploadTurn = 0;

        std::cout
            << "Scene GPU resources released"
            << std::endl;
    }

    // Прежний синхронный интерфейс.
    std::shared_ptr<EngineMesh> LoadMesh(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        auto found = meshes.find(path);

        if (found != meshes.end()) {
            return found->second;
        }

        const std::string binaryPath = GetBinaryMeshPath(path);
        auto data = BinaryMeshLoader::LoadMesh(binaryPath);

        if (!data) {
            std::cout
                << "Binary mesh not found, loading source mesh: "
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

            entry.future = SubmitResourceTask(
                [path, binaryPath]() -> std::shared_ptr<MeshData> {
                    ZoneScopedN("Resources::MeshCPU");
                    auto data = BinaryMeshLoader::LoadMesh(binaryPath);

                    if (!data) {
                        data = MeshLoader::LoadMesh(path);
                    }

                    return data;
                }
            );

            asyncMeshes.emplace(path, std::move(entry));
            pendingMeshUploads.push_back(path);

            std::cout
                << "Mesh queued in JobSystem: "
                << path << std::endl;
        }

        return GetPlaceholderMesh(renderer);
    }

    // Общий мягкий бюджет финализации моделей,
    // текстур и материалов.
    // Одна начатая GPU-операция может превысить бюджет.
    void PumpUploads(
        RenderAdapter& renderer,
        std::size_t maxUploads = 2,
        double budgetMilliseconds = 2.0
    ) {
        if (maxUploads == 0 || budgetMilliseconds <= 0.0) {
            return;
        }

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();

        const auto remainingMilliseconds = [&]() {
            return budgetMilliseconds -
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();
        };

        std::size_t completed = 0;
        std::size_t turn = uploadTurn;

        // Меняем первую очередь каждый кадр.
        uploadTurn = (uploadTurn + 1) % 3;

        while (completed < maxUploads) {
            std::size_t completedThisRound = 0;

            for (int i = 0; i < 3 && completed < maxUploads; ++i) {
                const double remaining = remainingMilliseconds();

                if (remaining <= 0.0) {
                    return;
                }

                std::size_t count = 0;

                switch (turn) {
                case 0:
                    count = PumpMeshUploads(renderer, 1, remaining);
                    break;

                case 1:
                    count = PumpTextureUploads(renderer, 1, remaining);
                    break;

                case 2:
                    count = PumpMaterials(renderer, 1, remaining);
                    break;
                }

                completed += count;
                completedThisRound += count;
                turn = (turn + 1) % 3;
            }

            if (completedThisRound == 0) {
                break;
            }
        }
    }

    std::size_t PumpMeshUploads(
        RenderAdapter& renderer,
        std::size_t maxUploads = 2,
        double budgetMilliseconds = 2.0
    ) {
        if (maxUploads == 0 || budgetMilliseconds <= 0.0) {
            return 0;
        }

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();

        std::size_t checksRemaining = pendingMeshUploads.size();
        std::size_t uploaded = 0;

        while (
            checksRemaining > 0 &&
            !pendingMeshUploads.empty() &&
            uploaded < maxUploads
        ) {
            const double elapsedMilliseconds =
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();

            if (elapsedMilliseconds >= budgetMilliseconds) {
                break;
            }

            --checksRemaining;

            std::string path = std::move(
                pendingMeshUploads.front()
            );
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
                std::cerr
                    << "Mesh job failed: "
                    << path << " | "
                    << error.what() << std::endl;
            }
            catch (...) {
                std::cerr
                    << "Mesh job failed: "
                    << path << " | unknown exception"
                    << std::endl;
            }

            asyncMeshes.erase(entry);

            if (!data) {
                failedMeshes.insert(path);

                std::cerr
                    << "Mesh unavailable: "
                    << path << std::endl;
                continue;
            }

            auto mesh = std::make_shared<EngineMesh>();
            mesh->Data = data;
            mesh->GPUHandle = renderer.UploadMesh(*data);

            meshes[path] = mesh;
            ++uploaded;

            std::cout
                << "Mesh uploaded on main thread: "
                << path << std::endl;
        }

        return uploaded;
    }

    // Название прежнее, но чтение и декодирование
    // теперь выполняются через JobSystem.
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

        auto texture = std::make_shared<EngineTexture>();
        texture->Data = placeholder->Data;
        texture->GPUHandle = placeholder->GPUHandle;

        auto future = SubmitResourceTask(
            [path]() -> std::shared_ptr<TextureData> {
                ZoneScopedN("Resources::TextureCPU");
                return TextureLoader::LoadTexture(path);
            }
        );

        textures.emplace(path, texture);
        asyncTextures.emplace(path, std::move(future));
        pendingTextureUploads.push_back(path);

        std::cout
            << "Texture queued in JobSystem: "
            << path << std::endl;

        return texture;
    }

    std::size_t PumpTextureUploads(
        RenderAdapter& renderer,
        std::size_t maxUploads = 2,
        double budgetMilliseconds = 2.0
    ) {
        if (maxUploads == 0 || budgetMilliseconds <= 0.0) {
            return 0;
        }

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();

        std::size_t checksRemaining = pendingTextureUploads.size();
        std::size_t uploaded = 0;

        while (
            checksRemaining > 0 &&
            !pendingTextureUploads.empty() &&
            uploaded < maxUploads
        ) {
            const double elapsedMilliseconds =
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();

            if (elapsedMilliseconds >= budgetMilliseconds) {
                break;
            }

            --checksRemaining;

            std::string path = std::move(
                pendingTextureUploads.front()
            );
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
                std::cerr
                    << "Texture job failed: "
                    << path << " | "
                    << error.what() << std::endl;
            }
            catch (...) {
                std::cerr
                    << "Texture job failed: "
                    << path << " | unknown exception"
                    << std::endl;
            }

            asyncTextures.erase(pending);

            if (!data) {
                std::cerr
                    << "Texture unavailable: "
                    << path << std::endl;
                continue;
            }

            auto target = textures.find(path);

            if (target == textures.end()) {
                continue;
            }

            const GPUTexture gpuTexture =
                renderer.CreateTexture(*data);

            ++uploaded;

            if (gpuTexture.ID == 0) {
                std::cerr
                    << "Texture GPU upload failed: "
                    << path << std::endl;
                continue;
            }

            // Меняем содержимое того же объекта:
            // материалы сохраняют корректные ссылки.
            target->second->Data = std::move(data);
            target->second->GPUHandle = gpuTexture;

            std::cout
                << "Texture uploaded on main thread: "
                << path << std::endl;
        }

        return uploaded;
    }

    // Прежний синхронный интерфейс загрузки шейдера.
    std::shared_ptr<EngineShaderProgram> LoadShader(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        RenderAdapter& renderer
    ) {
        const std::string key = vertexPath + "|" + fragmentPath;
        auto found = shaders.find(key);

        if (found != shaders.end()) {
            return found->second;
        }

        auto source = ShaderLoader::LoadShaderSource(
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

        shader->VertexLastWriteTime =
            std::filesystem::last_write_time(vertexPath);

        shader->FragmentLastWriteTime =
            std::filesystem::last_write_time(fragmentPath);

        shaders[key] = shader;
        return shader;
    }

    std::shared_ptr<EngineMaterial> LoadMaterial(
        const std::string& path,
        RenderAdapter& renderer
    ) {
        (void)renderer;

        auto found = materials.find(path);

        if (found != materials.end()) {
            return found->second;
        }

        if (!jobSystem) {
            throw std::runtime_error(
                "ResourceManager: JobSystem is not connected"
            );
        }

        auto material = std::make_shared<EngineMaterial>();

        auto future = SubmitResourceTask(
            [path]() -> PreparedMaterial {
                ZoneScopedN("Resources::MaterialCPU");
                PreparedMaterial result;

                result.data = MaterialLoader::LoadMaterial(path);

                if (!result.data) {
                    throw std::runtime_error(
                        "Cannot read material: " + path
                    );
                }

                const auto& vertexPath =
                    result.data->vertexShaderPath;

                const auto& fragmentPath =
                    result.data->fragmentShaderPath;

                if (!vertexPath.empty() && !fragmentPath.empty()) {
                    result.shaderSource =
                        ShaderLoader::LoadShaderSource(
                            vertexPath,
                            fragmentPath
                        );

                    if (!result.shaderSource) {
                        throw std::runtime_error(
                            "Cannot read material shaders: " + path
                        );
                    }

                    result.vertexTime =
                        std::filesystem::last_write_time(vertexPath);

                    result.fragmentTime =
                        std::filesystem::last_write_time(fragmentPath);
                }

                return result;
            }
        );

        materials.emplace(path, material);
        asyncMaterials.emplace(path, std::move(future));
        pendingMaterials.push_back(path);

        std::cout
            << "Material queued in JobSystem: "
            << path << std::endl;

        return material;
    }

    std::size_t PumpMaterials(
        RenderAdapter& renderer,
        std::size_t maxCompletions = 1,
        double budgetMilliseconds = 2.0
    ) {
        if (maxCompletions == 0 || budgetMilliseconds <= 0.0) {
            return 0;
        }

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();

        std::size_t checksRemaining = pendingMaterials.size();
        std::size_t completed = 0;

        while (
            checksRemaining > 0 &&
            !pendingMaterials.empty() &&
            completed < maxCompletions
        ) {
            const double elapsed =
                std::chrono::duration<double, std::milli>(
                    Clock::now() - started
                ).count();

            if (elapsed >= budgetMilliseconds) {
                break;
            }

            --checksRemaining;

            std::string path = std::move(
                pendingMaterials.front()
            );
            pendingMaterials.pop_front();

            auto pending = asyncMaterials.find(path);

            if (pending == asyncMaterials.end()) {
                continue;
            }

            const auto status = pending->second.wait_for(
                std::chrono::seconds(0)
            );

            if (status != std::future_status::ready) {
                pendingMaterials.push_back(std::move(path));
                continue;
            }

            PreparedMaterial prepared;
            bool succeeded = false;

            try {
                prepared = pending->second.get();
                succeeded = true;
            }
            catch (const std::exception& error) {
                std::cerr
                    << "Material job failed: "
                    << path << " | "
                    << error.what() << std::endl;
            }
            catch (...) {
                std::cerr
                    << "Material job failed: "
                    << path << " | unknown exception"
                    << std::endl;
            }

            asyncMaterials.erase(pending);
            ++completed;

            if (!succeeded || !prepared.data) {
                continue;
            }

            auto target = materials.find(path);

            if (target == materials.end()) {
                continue;
            }

            auto& material = target->second;
            material->Data = prepared.data;

            if (!prepared.data->texturePath.empty()) {
                material->Texture = LoadTexture(
                    prepared.data->texturePath,
                    renderer
                );
            }

            if (prepared.shaderSource) {
                const std::string key =
                    prepared.data->vertexShaderPath + "|" +
                    prepared.data->fragmentShaderPath;

                auto existing = shaders.find(key);

                if (existing != shaders.end()) {
                    material->Shader = existing->second;
                }
                else {
                    auto shader =
                        std::make_shared<EngineShaderProgram>();

                    shader->Source = prepared.shaderSource;

                    shader->VertexPath =
                        prepared.data->vertexShaderPath;

                    shader->FragmentPath =
                        prepared.data->fragmentShaderPath;

                    shader->VertexLastWriteTime =
                        prepared.vertexTime;

                    shader->FragmentLastWriteTime =
                        prepared.fragmentTime;

                    shader->GPUHandle = renderer.CreateShaderProgram(
                        prepared.shaderSource->VertexSource,
                        prepared.shaderSource->FragmentSource
                    );

                    if (shader->GPUHandle.ID == 0) {
                        std::cerr
                            << "Material shader compilation failed: "
                            << path << std::endl;
                        continue;
                    }

                    shaders.emplace(key, shader);
                    material->Shader = shader;
                }
            }

            std::cout
                << "Material finalized on main thread: "
                << path << std::endl;
        }

        return completed;
    }

    // Существующая перезагрузка изменённых шейдеров.
    // В этой версии она пока синхронная.
    void ReloadChangedShaders(RenderAdapter& renderer) {
        for (auto& pair : shaders) {
            auto& shader = pair.second;

            if (!shader) {
                continue;
            }

            if (
                !std::filesystem::exists(shader->VertexPath) ||
                !std::filesystem::exists(shader->FragmentPath)
            ) {
                continue;
            }

            const auto currentVertexTime =
                std::filesystem::last_write_time(
                    shader->VertexPath
                );

            const auto currentFragmentTime =
                std::filesystem::last_write_time(
                    shader->FragmentPath
                );

            const bool changed =
                currentVertexTime != shader->VertexLastWriteTime ||
                currentFragmentTime != shader->FragmentLastWriteTime;

            if (!changed) {
                continue;
            }

            std::cout
                << "Shader changed, reloading: "
                << shader->VertexPath << " | "
                << shader->FragmentPath << std::endl;

            auto newSource = ShaderLoader::LoadShaderSource(
                shader->VertexPath,
                shader->FragmentPath
            );

            if (!newSource) {
                std::cout
                    << "Shader reload failed: source loading error"
                    << std::endl;
                continue;
            }

            const GPUShaderProgram newProgram =
                renderer.CreateShaderProgram(
                    newSource->VertexSource,
                    newSource->FragmentSource
                );

            if (newProgram.ID == 0) {
                std::cout
                    << "Shader reload failed: compilation error"
                    << std::endl;
                continue;
            }

            renderer.DeleteShaderProgram(shader->GPUHandle);

            shader->Source = newSource;
            shader->GPUHandle = newProgram;
            shader->VertexLastWriteTime = currentVertexTime;
            shader->FragmentLastWriteTime = currentFragmentTime;

            std::cout
                << "Shader reloaded successfully"
                << std::endl;
        }
    }
};