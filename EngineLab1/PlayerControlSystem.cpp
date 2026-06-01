#include "PlayerControlSystem.h"

#include "InputManager.h"
#include "World.h"

#include <algorithm>
#include <cmath>

namespace {
    Vec3 AbsVec3(const Vec3& value) {
        return { std::fabs(value.x), std::fabs(value.y), std::fabs(value.z) };
    }

    float Max3(float first, float second, float third) {
        return std::max(first, std::max(second, third));
    }

    bool HorizontalOverlap(const AABB& first, const AABB& second) {
        const float overlapX = (first.halfSize.x + second.halfSize.x) -
            std::fabs(first.center.x - second.center.x);
        const float overlapZ = (first.halfSize.z + second.halfSize.z) -
            std::fabs(first.center.z - second.center.z);

        return overlapX > 0.0f && overlapZ > 0.0f;
    }
}

void PlayerControlSystem::Update(World& world, InputManager& input, float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }

    for (Entity entity : world.GetEntities()) {
        if (!world.HasTransform(entity) ||
            !world.HasRigidbody(entity) ||
            !world.HasCollider(entity) ||
            !world.HasPlayerController(entity)) {
            continue;
        }

        PlayerControllerComponent& controller = world.GetPlayerController(entity);
        RigidbodyComponent& rigidbody = world.GetRigidbody(entity);

        controller.grounded = IsGrounded(world, entity);

        float moveX = 0.0f;
        float moveZ = 0.0f;

        if (input.IsActionActive("PlayerForward")) {
            moveZ -= 1.0f;
        }
        if (input.IsActionActive("PlayerBackward")) {
            moveZ += 1.0f;
        }
        if (input.IsActionActive("PlayerLeft")) {
            moveX -= 1.0f;
        }
        if (input.IsActionActive("PlayerRight")) {
            moveX += 1.0f;
        }

        const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        rigidbody.velocity.x = moveX * controller.moveSpeed;
        rigidbody.velocity.z = moveZ * controller.moveSpeed;

        if (input.IsActionPressed("PlayerJump") && controller.grounded) {
            rigidbody.velocity.y = controller.jumpImpulse;
            controller.grounded = false;
        }
    }
}

AABB PlayerControlSystem::BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const {
    AABB box;

    box.center = {
        transform.position.x + collider.offset.x,
        transform.position.y + collider.offset.y,
        transform.position.z + collider.offset.z
    };

    Vec3 scale = AbsVec3(transform.scale);

    if (collider.type == ColliderType::SphereCollider) {
        float radius = collider.halfExtents.x * Max3(scale.x, scale.y, scale.z);
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

bool PlayerControlSystem::IsGrounded(World& world, Entity player) const {
    const AABB playerBox = BuildAABB(
        world.GetTransform(player),
        world.GetCollider(player)
    );

    const float playerBottom = playerBox.center.y - playerBox.halfSize.y;
    const float groundTolerance = 0.12f;

    for (Entity other : world.GetEntities()) {
        if (other == player || !world.HasTransform(other) || !world.HasCollider(other)) {
            continue;
        }

        if (world.GetCollider(other).isTrigger) {
            continue;
        }

        const AABB otherBox = BuildAABB(
            world.GetTransform(other),
            world.GetCollider(other)
        );

        const float otherTop = otherBox.center.y + otherBox.halfSize.y;
        const float verticalDistance = std::fabs(playerBottom - otherTop);

        if (verticalDistance <= groundTolerance && HorizontalOverlap(playerBox, otherBox)) {
            return true;
        }
    }

    return false;
}
