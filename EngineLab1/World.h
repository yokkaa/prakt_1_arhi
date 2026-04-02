#pragma once
#include "Entity.h"
#include "Components.h"

#include <unordered_map>
#include <vector>

class World {
public:
    Entity CreateEntity() {
        Entity entity = nextEntityId++;
        entities.push_back(entity);
        return entity;
    }

    const std::vector<Entity>& GetEntities() const {
        return entities;
    }

    void AddTransform(Entity entity, const TransformComponent& transform) {
        transforms[entity] = transform;
    }

    void AddTag(Entity entity, const Tag& tag) {
        tags[entity] = tag;
    }

    void AddMeshRenderer(Entity entity, const MeshRendererComponent& meshRenderer) {
        meshRenderers[entity] = meshRenderer;
    }

    bool HasTransform(Entity entity) const {
        return transforms.find(entity) != transforms.end();
    }

    bool HasTag(Entity entity) const {
        return tags.find(entity) != tags.end();
    }

    bool HasMeshRenderer(Entity entity) const {
        return meshRenderers.find(entity) != meshRenderers.end();
    }

    TransformComponent& GetTransform(Entity entity) {
        return transforms.at(entity);
    }

    Tag& GetTag(Entity entity) {
        return tags.at(entity);
    }

    MeshRendererComponent& GetMeshRenderer(Entity entity) {
        return meshRenderers.at(entity);
    }

    const TransformComponent& GetTransform(Entity entity) const {
        return transforms.at(entity);
    }

    const MeshRendererComponent& GetMeshRenderer(Entity entity) const {
        return meshRenderers.at(entity);
    }

private:
    Entity nextEntityId = 1;
    std::vector<Entity> entities;

    std::unordered_map<Entity, TransformComponent> transforms;
    std::unordered_map<Entity, Tag> tags;
    std::unordered_map<Entity, MeshRendererComponent> meshRenderers;
};