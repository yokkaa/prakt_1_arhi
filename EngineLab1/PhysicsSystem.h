#pragma once

#include "AABB.h"
#include "Entity.h"
#include "Components.h"

#include <utility>
#include <vector>

class World;
class EventBus;

class PhysicsSystem {
public:
    void Update(World& world, float deltaTime, EventBus* eventBus = nullptr);

    void SetGravity(float value) { gravityStrength = value; }
    float GetGravity() const { return gravityStrength; }

    void SetGridCellSize(float value) { gridCellSize = value; }
    float GetGridCellSize() const { return gridCellSize; }

private:
    struct CollisionInfo {
        bool collided = false;
        Vec3 normal{ 0.0f, 0.0f, 0.0f };
        float penetration = 0.0f;
    };

    float gravityStrength = 9.81f;
    float gridCellSize = 4.0f;

    void ApplyForcesAndIntegrate(World& world, float deltaTime);
    void CheckCollisions(World& world, EventBus* eventBus);

    std::vector<std::pair<Entity, Entity>> BuildBroadPhasePairs(World& world) const;

    AABB BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const;
    Vec3 GetColliderCenter(const TransformComponent& transform, const ColliderComponent& collider) const;
    float GetSphereRadius(const TransformComponent& transform, const ColliderComponent& collider) const;

    CollisionInfo DetectCollision(
        const TransformComponent& firstTransform,
        const ColliderComponent& firstCollider,
        const TransformComponent& secondTransform,
        const ColliderComponent& secondCollider
    ) const;

    CollisionInfo DetectAABBAABB(const AABB& first, const AABB& second) const;
    CollisionInfo DetectSphereSphere(
        const Vec3& firstCenter,
        float firstRadius,
        const Vec3& secondCenter,
        float secondRadius
    ) const;
    CollisionInfo DetectSphereAABB(
        const Vec3& sphereCenter,
        float sphereRadius,
        const AABB& box
    ) const;

    void ResolveCollision(
        World& world,
        Entity first,
        Entity second,
        const CollisionInfo& collision,
        EventBus* eventBus
    );
};
