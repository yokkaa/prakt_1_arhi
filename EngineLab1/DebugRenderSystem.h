#pragma once

#include "AABB.h"
#include "Components.h"
#include "Entity.h"

class World;
class RenderAdapter;

class DebugRenderSystem {
public:
    void RenderColliders(const World& world, RenderAdapter& renderer);

private:
    AABB BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const;
};
