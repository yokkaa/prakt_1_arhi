#include "DebugRenderSystem.h"

#include "RenderAdapter.h"
#include "World.h"

#include <algorithm>
#include <cmath>

namespace {
    Vec3 Add(const Vec3& first, const Vec3& second) {
        return {
            first.x + second.x,
            first.y + second.y,
            first.z + second.z
        };
    }

    Vec3 AbsVec3(const Vec3& value) {
        return {
            std::fabs(value.x),
            std::fabs(value.y),
            std::fabs(value.z)
        };
    }

    float Max3(float first, float second, float third) {
        return std::max(first, std::max(second, third));
    }
}

void DebugRenderSystem::RenderColliders(const World& world, RenderAdapter& renderer) {
    const Color4 dynamicColor{ 255, 60, 60, 180 };
    const Color4 staticColor{ 60, 120, 255, 180 };
    const Color4 triggerColor{ 255, 180, 40, 220 };
    const Color4 sphereColor{ 90, 220, 90, 220 };

    for (Entity entity : world.GetEntities()) {
        if (!world.HasTransform(entity) || !world.HasCollider(entity)) {
            continue;
        }

        const TransformComponent& transform = world.GetTransform(entity);
        const ColliderComponent& collider = world.GetCollider(entity);
        const Color4 baseColor = collider.isTrigger
            ? triggerColor
            : (world.HasRigidbody(entity) ? dynamicColor : staticColor);

        if (collider.type == ColliderType::SphereCollider) {
            const Vec3 scale = AbsVec3(transform.scale);
            const float radius = collider.halfExtents.x * Max3(scale.x, scale.y, scale.z);
            const Vec3 center = Add(transform.position, collider.offset);
            renderer.DrawDebugSphere(center, radius, collider.isTrigger ? triggerColor : sphereColor);
        }
        else {
            const AABB box = BuildAABB(transform, collider);
            renderer.DrawDebugAABB(box, baseColor);
        }
    }
}

AABB DebugRenderSystem::BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const {
    AABB box;
    box.center = Add(transform.position, collider.offset);

    const Vec3 scale = AbsVec3(transform.scale);

    if (collider.type == ColliderType::SphereCollider) {
        const float radius = collider.halfExtents.x * Max3(scale.x, scale.y, scale.z);
        box.halfSize = { radius, radius, radius };
    }
    else {
        box.halfSize = {
            collider.halfExtents.x * scale.x,
            collider.halfExtents.y * scale.y,
            collider.halfExtents.z * scale.z
        };
    }

    return box;
}
