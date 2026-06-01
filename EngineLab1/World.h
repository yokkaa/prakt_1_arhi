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

    void AddCamera(Entity entity, const CameraComponent& camera) {
        cameras[entity] = camera;
    }

    void AddRigidbody(Entity entity, const RigidbodyComponent& rigidbody) {
        rigidbodies[entity] = rigidbody;
    }

    void AddPlayerController(Entity entity, const PlayerControllerComponent& playerController) {
        playerControllers[entity] = playerController;
    }

    void AddCollider(Entity entity, const ColliderComponent& collider) {
        colliders[entity] = collider;
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

    bool HasCamera(Entity entity) const {
        return cameras.find(entity) != cameras.end();
    }

    bool HasRigidbody(Entity entity) const {
        return rigidbodies.find(entity) != rigidbodies.end();
    }

    bool HasPlayerController(Entity entity) const {
        return playerControllers.find(entity) != playerControllers.end();
    }

    bool HasCollider(Entity entity) const {
        return colliders.find(entity) != colliders.end();
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

    CameraComponent& GetCamera(Entity entity) {
        return cameras.at(entity);
    }

    RigidbodyComponent& GetRigidbody(Entity entity) {
        return rigidbodies.at(entity);
    }

    PlayerControllerComponent& GetPlayerController(Entity entity) {
        return playerControllers.at(entity);
    }

    ColliderComponent& GetCollider(Entity entity) {
        return colliders.at(entity);
    }

    const TransformComponent& GetTransform(Entity entity) const {
        return transforms.at(entity);
    }

    const MeshRendererComponent& GetMeshRenderer(Entity entity) const {
        return meshRenderers.at(entity);
    }

    const CameraComponent& GetCamera(Entity entity) const {
        return cameras.at(entity);
    }

    const Tag& GetTag(Entity entity) const {
        return tags.at(entity);
    }

    const RigidbodyComponent& GetRigidbody(Entity entity) const {
        return rigidbodies.at(entity);
    }

    const PlayerControllerComponent& GetPlayerController(Entity entity) const {
        return playerControllers.at(entity);
    }

    const ColliderComponent& GetCollider(Entity entity) const {
        return colliders.at(entity);
    }

private:
    Entity nextEntityId = 1;
    std::vector<Entity> entities;

    std::unordered_map<Entity, TransformComponent> transforms;
    std::unordered_map<Entity, Tag> tags;
    std::unordered_map<Entity, MeshRendererComponent> meshRenderers;
    std::unordered_map<Entity, CameraComponent> cameras;
    std::unordered_map<Entity, RigidbodyComponent> rigidbodies;
    std::unordered_map<Entity, PlayerControllerComponent> playerControllers;
    std::unordered_map<Entity, ColliderComponent> colliders;
};