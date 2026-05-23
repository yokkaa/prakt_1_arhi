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
    }

    std::cout << "Scene loaded: " << path << std::endl;

    return true;
}