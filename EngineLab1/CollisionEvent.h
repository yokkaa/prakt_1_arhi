#pragma once

#include "Entity.h"
#include "Components.h"
#include "EventBus.h"

struct CollisionEvent : public IEvent {
    Entity entityA = INVALID_ENTITY;
    Entity entityB = INVALID_ENTITY;
    Vec3 normal{ 0.0f, 0.0f, 0.0f };
    float penetrationDepth = 0.0f;
};
