#include "RenderSystem.h"

#include "RenderAdapter.h"
#include "ResourceManager.h"

void RenderSystem::Update(
    World& world,
    RenderAdapter& renderer,
    ResourceManager& resourceManager,
    float deltaTime
) {
    (void)deltaTime;

    for (Entity entity : world.GetEntities()) {
        if (!world.HasTransform(entity) || !world.HasMeshRenderer(entity)) {
            continue;
        }

        const TransformComponent& transform = world.GetTransform(entity);
        const MeshRendererComponent& meshRenderer = world.GetMeshRenderer(entity);

        auto mesh = resourceManager.LoadMeshAsync(
            meshRenderer.meshPath,
            renderer
        );

        auto material = resourceManager.LoadMaterial(
            meshRenderer.materialPath,
            renderer
        );

        if (!mesh || !material || !material->Texture || !material->Shader) {
            continue;
        }

        renderer.DrawMesh(
            mesh->GPUHandle,
            material->Texture->GPUHandle,
            material->Shader->GPUHandle,
            transform
        );
    }
}