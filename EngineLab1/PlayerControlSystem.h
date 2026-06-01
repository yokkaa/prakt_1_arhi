#pragma once

#include "AABB.h"
#include "Components.h"
#include "Entity.h"

class World;
class InputManager;

class PlayerControlSystem {
public:
    void Update(World& world, InputManager& input, float deltaTime);

private:
    AABB BuildAABB(const TransformComponent& transform, const ColliderComponent& collider) const;
    bool IsGrounded(World& world, Entity player) const;
};
