#pragma once

#include "Entity.h"
#include "Components.h"
#include "EventBus.h"

struct TriggerEvent : public IEvent {
    Entity triggerEntity = INVALID_ENTITY;
    Entity otherEntity = INVALID_ENTITY;
    Vec3 normal{ 0.0f, 0.0f, 0.0f };
};
