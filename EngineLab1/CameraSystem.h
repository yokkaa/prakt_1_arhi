#pragma once

class World;
class InputManager;
class RenderAdapter;

class CameraSystem {
public:
    void Update(World& world, InputManager& input, RenderAdapter& renderer, float deltaTime);

private:
    float moveSpeed = 5.0f;
    float fastMoveMultiplier = 3.0f;
    float mouseSensitivity = 0.12f;
};
