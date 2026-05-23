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