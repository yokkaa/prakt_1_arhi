#pragma once

#include <string>

enum class PrimitiveType {
    Triangle,
    Rectangle
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Color4 {
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    unsigned char a = 255;
};

struct TransformComponent {
    Vec3 position{ 0.0f, 0.0f, 0.0f };
    Vec3 rotation{ 0.0f, 0.0f, 0.0f };
    Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

struct Tag {
    std::string name;
};

struct MeshRendererComponent {
    std::string meshPath;
    std::string materialPath;
};

struct CameraComponent {
    Vec3 position{ 0.0f, 2.0f, 6.0f };
    Vec3 forward{ 0.0f, -0.25f, -1.0f };
    Vec3 up{ 0.0f, 1.0f, 0.0f };
    float fov = 45.0f;
    float aspectRatio = 800.0f / 600.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float yaw = -90.0f;
    float pitch = -10.0f;
    bool active = true;
};

struct RigidbodyComponent {
    Vec3 velocity{ 0.0f, 0.0f, 0.0f };
    Vec3 acceleration{ 0.0f, 0.0f, 0.0f };
    float mass = 1.0f;
    bool useGravity = true;
};

struct PlayerControllerComponent {
    float moveSpeed = 5.0f;
    float jumpImpulse = 6.0f;
    bool grounded = false;
};

enum class ColliderType {
    BoxCollider,
    SphereCollider
};

struct ColliderComponent {
    ColliderType type = ColliderType::BoxCollider;
    Vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
    Vec3 offset{ 0.0f, 0.0f, 0.0f };
    float bounciness = 0.1f;
    float friction = 0.2f;
    bool isTrigger = false;
};