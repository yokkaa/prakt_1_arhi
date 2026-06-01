#include "PhysicsSystem.h"

#include "World.h"
#include "EventBus.h"
#include "CollisionEvent.h"
#include "TriggerEvent.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
    Vec3 Add(const Vec3& first, const Vec3& second) {
        return { first.x + second.x, first.y + second.y, first.z + second.z };
    }

    Vec3 Subtract(const Vec3& first, const Vec3& second) {
        return { first.x - second.x, first.y - second.y, first.z - second.z };
    }

    Vec3 Multiply(const Vec3& value, float scalar) {
        return { value.x * scalar, value.y * scalar, value.z * scalar };
    }

    float Dot(const Vec3& first, const Vec3& second) {
        return first.x * second.x + first.y * second.y + first.z * second.z;
    }

    float LengthSquared(const Vec3& value) {
        return Dot(value, value);
    }

    float Length(const Vec3& value) {
        return std::sqrt(LengthSquared(value));
    }

    Vec3 Normalize(const Vec3& value) {
        const float length = Length(value);
        if (length <= 0.0001f) {
            return { 0.0f, 1.0f, 0.0f };
        }

        return Multiply(value, 1.0f / length);
    }

    Vec3 AbsVec3(const Vec3& value) {
        return { std::fabs(value.x), std::fabs(value.y), std::fabs(value.z) };
    }

    float Max3(float first, float second, float third) {
        return std::max(first, std::max(second, third));
    }

    float Clamp(float value, float minValue, float maxValue) {
        return std::max(minValue, std::min(value, maxValue));
    }

    struct GridCell {
        int x = 0;
        int y = 0;
        int z = 0;

        bool operator==(const GridCell& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct GridCellHash {
        std::size_t operator()(const GridCell& cell) const {
            const std::size_t xHash = std::hash<int>{}(cell.x * 73856093);
            const std::size_t yHash = std::hash<int>{}(cell.y * 19349663);
            const std::size_t zHash = std::hash<int>{}(cell.z * 83492791);
            return xHash ^ (yHash << 1) ^ (zHash << 2);
        }
    };

    std::uint64_t MakePairKey(Entity first, Entity second) {
        if (first > second) {
            std::swap(first, second);
        }

        return (static_cast<std::uint64_t>(first) << 32) | static_cast<std::uint64_t>(second);
    }

    float InverseMass(World& world, Entity entity) {
        if (!world.HasRigidbody(entity)) {
            return 0.0f;
        }

        const float mass = world.GetRigidbody(entity).mass;
        if (mass <= 0.0001f) {
            return 0.0f;
        }

        return 1.0f / mass;
    }
}

void PhysicsSystem::Update(World& world, float deltaTime, EventBus* eventBus) {
    if (deltaTime <= 0.0f) {
        return;
    }

    ApplyForcesAndIntegrate(world, deltaTime);
    CheckCollisions(world, eventBus);
}

void PhysicsSystem::ApplyForcesAndIntegrate(World& world, float deltaTime) {
    for (Entity entity : world.GetEntities()) {
        if (!world.HasTransform(entity) || !world.HasRigidbody(entity)) {
            continue;
        }

        TransformComponent& transform = world.GetTransform(entity);
        RigidbodyComponent& rigidbody = world.GetRigidbody(entity);

        if (rigidbody.useGravity) {
            rigidbody.acceleration.y -= gravityStrength;
        }

        rigidbody.velocity.x += rigidbody.acceleration.x * deltaTime;
        rigidbody.velocity.y += rigidbody.acceleration.y * deltaTime;
        rigidbody.velocity.z += rigidbody.acceleration.z * deltaTime;

        transform.position.x += rigidbody.velocity.x * deltaTime;
        transform.position.y += rigidbody.velocity.y * deltaTime;
        transform.position.z += rigidbody.velocity.z * deltaTime;

        rigidbody.acceleration = { 0.0f, 0.0f, 0.0f };
    }
}

void PhysicsSystem::CheckCollisions(World& world, EventBus* eventBus) {
    const std::vector<std::pair<Entity, Entity>> candidatePairs = BuildBroadPhasePairs(world);

    for (const auto& pair : candidatePairs) {
        const Entity first = pair.first;
        const Entity second = pair.second;

        const CollisionInfo collision = DetectCollision(
            world.GetTransform(first),
            world.GetCollider(first),
            world.GetTransform(second),
            world.GetCollider(second)
        );

        if (collision.collided) {
            ResolveCollision(world, first, second, collision, eventBus);
        }
    }
}

std::vector<std::pair<Entity, Entity>> PhysicsSystem::BuildBroadPhasePairs(World& world) const {
    std::vector<Entity> collidableEntities;

    for (Entity entity : world.GetEntities()) {
        if (world.HasTransform(entity) && world.HasCollider(entity)) {
            collidableEntities.push_back(entity);
        }
    }

    std::unordered_map<GridCell, std::vector<Entity>, GridCellHash> grid;

    for (Entity entity : collidableEntities) {
        const AABB box = BuildAABB(world.GetTransform(entity), world.GetCollider(entity));

        const int minX = static_cast<int>(std::floor((box.center.x - box.halfSize.x) / gridCellSize));
        const int maxX = static_cast<int>(std::floor((box.center.x + box.halfSize.x) / gridCellSize));
        const int minY = static_cast<int>(std::floor((box.center.y - box.halfSize.y) / gridCellSize));
        const int maxY = static_cast<int>(std::floor((box.center.y + box.halfSize.y) / gridCellSize));
        const int minZ = static_cast<int>(std::floor((box.center.z - box.halfSize.z) / gridCellSize));
        const int maxZ = static_cast<int>(std::floor((box.center.z + box.halfSize.z) / gridCellSize));

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    grid[GridCell{ x, y, z }].push_back(entity);
                }
            }
        }
    }

    std::vector<std::pair<Entity, Entity>> pairs;
    std::unordered_set<std::uint64_t> usedPairs;

    for (const auto& cellEntry : grid) {
        const std::vector<Entity>& cellEntities = cellEntry.second;

        for (std::size_t i = 0; i < cellEntities.size(); ++i) {
            for (std::size_t j = i + 1; j < cellEntities.size(); ++j) {
                Entity first = cellEntities[i];
                Entity second = cellEntities[j];

                if (first > second) {
                    std::swap(first, second);
                }

                const std::uint64_t key = MakePairKey(first, second);
                if (usedPairs.insert(key).second) {
                    pairs.emplace_back(first, second);
                }
            }
        }
    }

    return pairs;
}

AABB PhysicsSystem::BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const {
    AABB box;
    box.center = GetColliderCenter(transform, collider);

    const Vec3 scale = AbsVec3(transform.scale);

    if (collider.type == ColliderType::SphereCollider) {
        const float radius = GetSphereRadius(transform, collider);
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

Vec3 PhysicsSystem::GetColliderCenter(const TransformComponent& transform, const ColliderComponent& collider) const {
    return Add(transform.position, collider.offset);
}

float PhysicsSystem::GetSphereRadius(const TransformComponent& transform, const ColliderComponent& collider) const {
    const Vec3 scale = AbsVec3(transform.scale);
    return collider.halfExtents.x * Max3(scale.x, scale.y, scale.z);
}

PhysicsSystem::CollisionInfo PhysicsSystem::DetectCollision(
    const TransformComponent& firstTransform,
    const ColliderComponent& firstCollider,
    const TransformComponent& secondTransform,
    const ColliderComponent& secondCollider
) const {
    if (firstCollider.type == ColliderType::SphereCollider &&
        secondCollider.type == ColliderType::SphereCollider) {
        return DetectSphereSphere(
            GetColliderCenter(firstTransform, firstCollider),
            GetSphereRadius(firstTransform, firstCollider),
            GetColliderCenter(secondTransform, secondCollider),
            GetSphereRadius(secondTransform, secondCollider)
        );
    }

    if (firstCollider.type == ColliderType::SphereCollider &&
        secondCollider.type == ColliderType::BoxCollider) {
        return DetectSphereAABB(
            GetColliderCenter(firstTransform, firstCollider),
            GetSphereRadius(firstTransform, firstCollider),
            BuildAABB(secondTransform, secondCollider)
        );
    }

    if (firstCollider.type == ColliderType::BoxCollider &&
        secondCollider.type == ColliderType::SphereCollider) {
        CollisionInfo collision = DetectSphereAABB(
            GetColliderCenter(secondTransform, secondCollider),
            GetSphereRadius(secondTransform, secondCollider),
            BuildAABB(firstTransform, firstCollider)
        );

        collision.normal = Multiply(collision.normal, -1.0f);
        return collision;
    }

    return DetectAABBAABB(
        BuildAABB(firstTransform, firstCollider),
        BuildAABB(secondTransform, secondCollider)
    );
}

PhysicsSystem::CollisionInfo PhysicsSystem::DetectAABBAABB(const AABB& first, const AABB& second) const {
    CollisionInfo collision;

    const float dx = first.center.x - second.center.x;
    const float dy = first.center.y - second.center.y;
    const float dz = first.center.z - second.center.z;

    const float overlapX = (first.halfSize.x + second.halfSize.x) - std::fabs(dx);
    const float overlapY = (first.halfSize.y + second.halfSize.y) - std::fabs(dy);
    const float overlapZ = (first.halfSize.z + second.halfSize.z) - std::fabs(dz);

    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) {
        return collision;
    }

    collision.collided = true;
    collision.penetration = overlapX;

    if (overlapX <= overlapY && overlapX <= overlapZ) {
        collision.normal = { dx < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f };
        collision.penetration = overlapX;
    }
    else if (overlapY <= overlapX && overlapY <= overlapZ) {
        collision.normal = { 0.0f, dy < 0.0f ? -1.0f : 1.0f, 0.0f };
        collision.penetration = overlapY;
    }
    else {
        collision.normal = { 0.0f, 0.0f, dz < 0.0f ? -1.0f : 1.0f };
        collision.penetration = overlapZ;
    }

    return collision;
}

PhysicsSystem::CollisionInfo PhysicsSystem::DetectSphereSphere(
    const Vec3& firstCenter,
    float firstRadius,
    const Vec3& secondCenter,
    float secondRadius
) const {
    CollisionInfo collision;

    const Vec3 delta = Subtract(firstCenter, secondCenter);
    const float radiusSum = firstRadius + secondRadius;
    const float distanceSquared = LengthSquared(delta);

    if (distanceSquared >= radiusSum * radiusSum) {
        return collision;
    }

    const float distance = std::sqrt(distanceSquared);
    collision.collided = true;
    collision.normal = distance > 0.0001f ? Multiply(delta, 1.0f / distance) : Vec3{ 0.0f, 1.0f, 0.0f };
    collision.penetration = radiusSum - distance;
    return collision;
}

PhysicsSystem::CollisionInfo PhysicsSystem::DetectSphereAABB(
    const Vec3& sphereCenter,
    float sphereRadius,
    const AABB& box
) const {
    CollisionInfo collision;

    const Vec3 closestPoint{
        Clamp(sphereCenter.x, box.center.x - box.halfSize.x, box.center.x + box.halfSize.x),
        Clamp(sphereCenter.y, box.center.y - box.halfSize.y, box.center.y + box.halfSize.y),
        Clamp(sphereCenter.z, box.center.z - box.halfSize.z, box.center.z + box.halfSize.z)
    };

    const Vec3 delta = Subtract(sphereCenter, closestPoint);
    const float distanceSquared = LengthSquared(delta);

    if (distanceSquared > sphereRadius * sphereRadius) {
        return collision;
    }

    collision.collided = true;

    if (distanceSquared > 0.0001f) {
        const float distance = std::sqrt(distanceSquared);
        collision.normal = Multiply(delta, 1.0f / distance);
        collision.penetration = sphereRadius - distance;
    }
    else {
        const float dx = sphereCenter.x - box.center.x;
        const float dy = sphereCenter.y - box.center.y;
        const float dz = sphereCenter.z - box.center.z;

        const float overlapX = box.halfSize.x + sphereRadius - std::fabs(dx);
        const float overlapY = box.halfSize.y + sphereRadius - std::fabs(dy);
        const float overlapZ = box.halfSize.z + sphereRadius - std::fabs(dz);

        if (overlapX <= overlapY && overlapX <= overlapZ) {
            collision.normal = { dx < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f };
            collision.penetration = overlapX;
        }
        else if (overlapY <= overlapX && overlapY <= overlapZ) {
            collision.normal = { 0.0f, dy < 0.0f ? -1.0f : 1.0f, 0.0f };
            collision.penetration = overlapY;
        }
        else {
            collision.normal = { 0.0f, 0.0f, dz < 0.0f ? -1.0f : 1.0f };
            collision.penetration = overlapZ;
        }
    }

    return collision;
}

void PhysicsSystem::ResolveCollision(
    World& world,
    Entity first,
    Entity second,
    const CollisionInfo& collision,
    EventBus* eventBus
) {
    const ColliderComponent& firstCollider = world.GetCollider(first);
    const ColliderComponent& secondCollider = world.GetCollider(second);

    if (firstCollider.isTrigger || secondCollider.isTrigger) {
        if (eventBus) {
            if (firstCollider.isTrigger) {
                TriggerEvent triggerEvent;
                triggerEvent.triggerEntity = first;
                triggerEvent.otherEntity = second;
                triggerEvent.normal = collision.normal;
                eventBus->Publish(triggerEvent);
            }

            if (secondCollider.isTrigger) {
                TriggerEvent triggerEvent;
                triggerEvent.triggerEntity = second;
                triggerEvent.otherEntity = first;
                triggerEvent.normal = Multiply(collision.normal, -1.0f);
                eventBus->Publish(triggerEvent);
            }
        }

        return;
    }

    const bool firstDynamic = world.HasRigidbody(first) && InverseMass(world, first) > 0.0f;
    const bool secondDynamic = world.HasRigidbody(second) && InverseMass(world, second) > 0.0f;

    if (!firstDynamic && !secondDynamic) {
        return;
    }

    if (eventBus) {
        CollisionEvent collisionEvent;
        collisionEvent.entityA = first;
        collisionEvent.entityB = second;
        collisionEvent.normal = collision.normal;
        collisionEvent.penetrationDepth = collision.penetration;
        eventBus->Publish(collisionEvent);
    }

    TransformComponent& firstTransform = world.GetTransform(first);
    TransformComponent& secondTransform = world.GetTransform(second);

    const float firstInvMass = InverseMass(world, first);
    const float secondInvMass = InverseMass(world, second);
    const float invMassSum = firstInvMass + secondInvMass;

    if (invMassSum <= 0.0f) {
        return;
    }

    const Vec3 correction = Multiply(collision.normal, collision.penetration / invMassSum);

    if (firstDynamic) {
        firstTransform.position = Add(firstTransform.position, Multiply(correction, firstInvMass));
    }
    if (secondDynamic) {
        secondTransform.position = Subtract(secondTransform.position, Multiply(correction, secondInvMass));
    }

    RigidbodyComponent* firstBody = firstDynamic ? &world.GetRigidbody(first) : nullptr;
    RigidbodyComponent* secondBody = secondDynamic ? &world.GetRigidbody(second) : nullptr;

    const Vec3 firstVelocity = firstBody ? firstBody->velocity : Vec3{};
    const Vec3 secondVelocity = secondBody ? secondBody->velocity : Vec3{};
    const Vec3 relativeVelocity = Subtract(firstVelocity, secondVelocity);
    const float velocityAlongNormal = Dot(relativeVelocity, collision.normal);

    if (velocityAlongNormal < 0.0f) {
        const float restitution = std::max(firstCollider.bounciness, secondCollider.bounciness);
        const float impulseMagnitude = -(1.0f + restitution) * velocityAlongNormal / invMassSum;
        const Vec3 impulse = Multiply(collision.normal, impulseMagnitude);

        if (firstBody) {
            firstBody->velocity = Add(firstBody->velocity, Multiply(impulse, firstInvMass));
        }
        if (secondBody) {
            secondBody->velocity = Subtract(secondBody->velocity, Multiply(impulse, secondInvMass));
        }
    }

    const float friction = Clamp((firstCollider.friction + secondCollider.friction) * 0.5f, 0.0f, 1.0f);

    if (firstBody) {
        const float normalVelocity = Dot(firstBody->velocity, collision.normal);
        Vec3 tangentVelocity = Subtract(firstBody->velocity, Multiply(collision.normal, normalVelocity));
        firstBody->velocity = Subtract(firstBody->velocity, Multiply(tangentVelocity, friction));

        if (std::fabs(Dot(firstBody->velocity, collision.normal)) < 0.05f) {
            firstBody->velocity = Subtract(
                firstBody->velocity,
                Multiply(collision.normal, Dot(firstBody->velocity, collision.normal))
            );
        }
    }

    if (secondBody) {
        const Vec3 secondNormal = Multiply(collision.normal, -1.0f);
        const float normalVelocity = Dot(secondBody->velocity, secondNormal);
        Vec3 tangentVelocity = Subtract(secondBody->velocity, Multiply(secondNormal, normalVelocity));
        secondBody->velocity = Subtract(secondBody->velocity, Multiply(tangentVelocity, friction));

        if (std::fabs(Dot(secondBody->velocity, secondNormal)) < 0.05f) {
            secondBody->velocity = Subtract(
                secondBody->velocity,
                Multiply(secondNormal, Dot(secondBody->velocity, secondNormal))
            );
        }
    }
}
