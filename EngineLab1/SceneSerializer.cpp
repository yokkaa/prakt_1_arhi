#include "SceneSerializer.h"

#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

static json Vec3ToJson(const Vec3& vec) {
    return {
        { "x", vec.x },
        { "y", vec.y },
        { "z", vec.z }
    };
}

static Vec3 JsonToVec3(const json& data) {
    Vec3 vec;

    vec.x = data.value("x", 0.0f);
    vec.y = data.value("y", 0.0f);
    vec.z = data.value("z", 0.0f);

    return vec;
}

static std::string ColliderTypeToString(ColliderType type) {
    switch (type) {
    case ColliderType::SphereCollider:
        return "sphere";
    case ColliderType::BoxCollider:
    default:
        return "box";
    }
}

static ColliderType JsonToColliderType(const json& data) {
    std::string type = data.get<std::string>();

    if (type == "sphere" || type == "SphereCollider" || type == "SPHERE_COLLIDER") {
        return ColliderType::SphereCollider;
    }

    return ColliderType::BoxCollider;
}

bool SceneSerializer::SaveScene(
    const std::string& path,
    World& world
) {
    json sceneJson;
    sceneJson["entities"] = json::array();

    for (Entity entity : world.GetEntities()) {
        json entityJson;

        if (world.HasTag(entity)) {
            const Tag& tag = world.GetTag(entity);
            entityJson["tag"] = tag.name;
        }

        if (world.HasTransform(entity)) {
            const TransformComponent& transform = world.GetTransform(entity);

            entityJson["transform"] = {
                { "position", Vec3ToJson(transform.position) },
                { "rotation", Vec3ToJson(transform.rotation) },
                { "scale", Vec3ToJson(transform.scale) }
            };
        }

        if (world.HasMeshRenderer(entity)) {
            const MeshRendererComponent& meshRenderer =
                world.GetMeshRenderer(entity);

            entityJson["meshRenderer"] = {
                { "mesh", meshRenderer.meshPath },
                { "material", meshRenderer.materialPath }
            };
        }

        if (world.HasCamera(entity)) {
            const CameraComponent& camera = world.GetCamera(entity);

            entityJson["camera"] = {
                { "position", Vec3ToJson(camera.position) },
                { "forward", Vec3ToJson(camera.forward) },
                { "up", Vec3ToJson(camera.up) },
                { "fov", camera.fov },
                { "aspectRatio", camera.aspectRatio },
                { "nearPlane", camera.nearPlane },
                { "farPlane", camera.farPlane },
                { "yaw", camera.yaw },
                { "pitch", camera.pitch },
                { "active", camera.active }
            };
        }

        if (world.HasRigidbody(entity)) {
            const RigidbodyComponent& rigidbody = world.GetRigidbody(entity);

            entityJson["rigidbody"] = {
                { "velocity", Vec3ToJson(rigidbody.velocity) },
                { "acceleration", Vec3ToJson(rigidbody.acceleration) },
                { "mass", rigidbody.mass },
                { "useGravity", rigidbody.useGravity }
            };
        }

        if (world.HasPlayerController(entity)) {
            const PlayerControllerComponent& playerController = world.GetPlayerController(entity);

            entityJson["playerController"] = {
                { "moveSpeed", playerController.moveSpeed },
                { "jumpImpulse", playerController.jumpImpulse },
                { "grounded", playerController.grounded }
            };
        }

        if (world.HasCollider(entity)) {
            const ColliderComponent& collider = world.GetCollider(entity);

            entityJson["collider"] = {
                { "type", ColliderTypeToString(collider.type) },
                { "halfExtents", Vec3ToJson(collider.halfExtents) },
                { "offset", Vec3ToJson(collider.offset) },
                { "bounciness", collider.bounciness },
                { "friction", collider.friction },
                { "isTrigger", collider.isTrigger }
            };
        }

        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to save scene: " << path << std::endl;
        return false;
    }

    file << sceneJson.dump(4);

    std::cout << "Scene saved: " << path << std::endl;

    return true;
}

bool SceneSerializer::LoadScene(
    const std::string& path,
    World& world,
    ResourceManager& resourceManager,
    RenderAdapter& renderer
) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to load scene: " << path << std::endl;
        return false;
    }

    json sceneJson;
    file >> sceneJson;

    if (!sceneJson.contains("entities") || !sceneJson["entities"].is_array()) {
        std::cerr << "Invalid scene file: " << path << std::endl;
        return false;
    }

    for (const auto& entityJson : sceneJson["entities"]) {
        Entity entity = world.CreateEntity();

        if (entityJson.contains("tag")) {
            Tag tag;
            tag.name = entityJson["tag"].get<std::string>();
            world.AddTag(entity, tag);
        }

        if (entityJson.contains("transform")) {
            const auto& transformJson = entityJson["transform"];

            TransformComponent transform;

            if (transformJson.contains("position")) {
                transform.position = JsonToVec3(transformJson["position"]);
            }

            if (transformJson.contains("rotation")) {
                transform.rotation = JsonToVec3(transformJson["rotation"]);
            }

            if (transformJson.contains("scale")) {
                transform.scale = JsonToVec3(transformJson["scale"]);
            }

            world.AddTransform(entity, transform);
        }

        if (entityJson.contains("meshRenderer")) {
            const auto& rendererJson = entityJson["meshRenderer"];

            MeshRendererComponent meshRenderer;

            meshRenderer.meshPath =
                rendererJson.value("mesh", "");

            meshRenderer.materialPath =
                rendererJson.value("material", "");

            world.AddMeshRenderer(entity, meshRenderer);

            if (!meshRenderer.meshPath.empty()) {
                resourceManager.LoadMeshAsync(meshRenderer.meshPath, renderer);
            }

            if (!meshRenderer.materialPath.empty()) {
                resourceManager.LoadMaterial(meshRenderer.materialPath, renderer);
            }
        }

        if (entityJson.contains("camera")) {
            const auto& cameraJson = entityJson["camera"];

            CameraComponent camera;

            if (cameraJson.contains("position")) {
                camera.position = JsonToVec3(cameraJson["position"]);
            }

            if (cameraJson.contains("forward")) {
                camera.forward = JsonToVec3(cameraJson["forward"]);
            }

            if (cameraJson.contains("up")) {
                camera.up = JsonToVec3(cameraJson["up"]);
            }

            camera.fov = cameraJson.value("fov", 45.0f);
            camera.aspectRatio = cameraJson.value("aspectRatio", 800.0f / 600.0f);
            camera.nearPlane = cameraJson.value("nearPlane", 0.1f);
            camera.farPlane = cameraJson.value("farPlane", 100.0f);
            camera.yaw = cameraJson.value("yaw", -90.0f);
            camera.pitch = cameraJson.value("pitch", -10.0f);
            camera.active = cameraJson.value("active", true);

            world.AddCamera(entity, camera);
        }

        if (entityJson.contains("rigidbody")) {
            const auto& rigidbodyJson = entityJson["rigidbody"];

            RigidbodyComponent rigidbody;

            if (rigidbodyJson.contains("velocity")) {
                rigidbody.velocity = JsonToVec3(rigidbodyJson["velocity"]);
            }

            if (rigidbodyJson.contains("acceleration")) {
                rigidbody.acceleration = JsonToVec3(rigidbodyJson["acceleration"]);
            }

            rigidbody.mass = rigidbodyJson.value("mass", 1.0f);
            rigidbody.useGravity = rigidbodyJson.value("useGravity", true);

            world.AddRigidbody(entity, rigidbody);
        }

        if (entityJson.contains("playerController")) {
            const auto& playerControllerJson = entityJson["playerController"];

            PlayerControllerComponent playerController;
            playerController.moveSpeed = playerControllerJson.value("moveSpeed", 5.0f);
            playerController.jumpImpulse = playerControllerJson.value("jumpImpulse", 6.0f);
            playerController.grounded = playerControllerJson.value("grounded", false);

            world.AddPlayerController(entity, playerController);
        }

        if (entityJson.contains("collider")) {
            const auto& colliderJson = entityJson["collider"];

            ColliderComponent collider;

            if (colliderJson.contains("type")) {
                collider.type = JsonToColliderType(colliderJson["type"]);
            }

            if (colliderJson.contains("halfExtents")) {
                collider.halfExtents = JsonToVec3(colliderJson["halfExtents"]);
            }

            if (colliderJson.contains("offset")) {
                collider.offset = JsonToVec3(colliderJson["offset"]);
            }

            collider.bounciness = colliderJson.value("bounciness", 0.1f);
            collider.friction = colliderJson.value("friction", 0.2f);
            collider.isTrigger = colliderJson.value("isTrigger", false);

            world.AddCollider(entity, collider);
        }
    }

    std::cout << "Scene loaded: " << path << std::endl;

    return true;
}