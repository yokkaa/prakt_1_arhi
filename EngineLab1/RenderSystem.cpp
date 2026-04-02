#include "RenderSystem.h"
#include "RenderAdapter.h"

void RenderSystem::Update(World& world, RenderAdapter& renderer, float deltaTime) {
    (void)deltaTime;

    for (Entity entity : world.GetEntities()) {
        if (!world.HasTransform(entity) || !world.HasMeshRenderer(entity)) {
            continue;
        }

        const TransformComponent& transform = world.GetTransform(entity);
        const MeshRendererComponent& meshRenderer = world.GetMeshRenderer(entity);

        if (meshRenderer.primitive == PrimitiveType::Triangle) {
            renderer.DrawTriangle(transform, meshRenderer.color);
        }
        else if (meshRenderer.primitive == PrimitiveType::Rectangle) {
            renderer.DrawRectangle(transform, meshRenderer.color);
        }
    }
}