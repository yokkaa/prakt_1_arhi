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

struct Color4 {
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    unsigned char a = 255;
};

struct TransformComponent {
    Vec2 position{ 0.0f, 0.0f };
    float rotation = 0.0f;
    Vec2 scale{ 1.0f, 1.0f };
};

struct Tag {
    std::string name;
};

struct MeshRendererComponent {
    PrimitiveType primitive = PrimitiveType::Triangle;
    Color4 color{ 255, 0, 0, 255 };
};