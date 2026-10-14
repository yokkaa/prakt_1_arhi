#pragma once

#include "World.h"
#include "PreparedModelMatrix.h"
#include "Mesh.h"
#include "Texture.h"
#include "ShaderProgram.h"

#include <cstddef>
#include <vector>

class RenderAdapter;
class ResourceManager;
class JobSystem;

class RenderSystem {
public:
    void Update(
        World& world,
        RenderAdapter& renderer,
        ResourceManager& resourceManager,
        float deltaTime,
        JobSystem& jobs,
        bool parallel
    );

    std::size_t GetObjectCount() const {
        return transforms.size();
    }

    std::size_t GetJobCount() const {
        return lastJobCount;
    }

private:
    // Данные для отрисовки одного объекта.
    struct DrawItem {
        GPUMesh mesh;
        GPUTexture texture;
        GPUShaderProgram shader;
    };

    // Копии трансформаций: рабочие потоки читают этот массив,
    // не обращаясь к самому миру.
    std::vector<TransformComponent> transforms;

    // Результаты расчётов рабочих потоков.
    std::vector<PreparedModelMatrix> matrices;

    // GPU-дескрипторы используются только главным потоком.
    std::vector<DrawItem> items;

    std::size_t lastJobCount = 0;
};