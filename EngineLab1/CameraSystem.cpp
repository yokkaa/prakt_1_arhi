#include "CameraSystem.h"

#include "World.h"
#include "InputManager.h"
#include "RenderAdapter.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float DegToRad = 3.1415926535f / 180.0f;

    Vec3 Add(const Vec3& a, const Vec3& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    Vec3 Subtract(const Vec3& a, const Vec3& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    Vec3 Scale(const Vec3& value, float scalar) {
        return { value.x * scalar, value.y * scalar, value.z * scalar };
    }

    float Length(const Vec3& value) {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    Vec3 Normalize(const Vec3& value) {
        const float length = Length(value);
        if (length <= 0.0001f) {
            return { 0.0f, 0.0f, 0.0f };
        }

        return {
            value.x / length,
            value.y / length,
            value.z / length
        };
    }

    Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    Vec3 ForwardFromYawPitch(float yaw, float pitch) {
        const float yawRad = yaw * DegToRad;
        const float pitchRad = pitch * DegToRad;

        return Normalize({
            std::cos(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            std::sin(yawRad) * std::cos(pitchRad)
        });
    }
}

void CameraSystem::Update(World& world, InputManager& input, RenderAdapter& renderer, float deltaTime) {
    for (Entity entity : world.GetEntities()) {
        if (!world.HasCamera(entity)) {
            continue;
        }

        CameraComponent& camera = world.GetCamera(entity);
        if (!camera.active) {
            continue;
        }

        const float speedMultiplier =
            (input.IsKeyDown(KeyCode::KEY_LEFT_SHIFT) || input.IsKeyDown(KeyCode::KEY_RIGHT_SHIFT))
            ? fastMoveMultiplier
            : 1.0f;

        const float frameSpeed = moveSpeed * speedMultiplier * deltaTime;
        const Vec3 forward = Normalize(camera.forward);
        const Vec3 right = Normalize(Cross(forward, camera.up));

        Vec3 movement{ 0.0f, 0.0f, 0.0f };

        if (input.IsActionActive("CameraForward")) {
            movement = Add(movement, forward);
        }

        if (input.IsActionActive("CameraBackward")) {
            movement = Subtract(movement, forward);
        }

        if (input.IsActionActive("CameraLeft")) {
            movement = Subtract(movement, right);
        }

        if (input.IsActionActive("CameraRight")) {
            movement = Add(movement, right);
        }

        if (input.IsActionActive("CameraUp")) {
            movement = Add(movement, camera.up);
        }

        if (input.IsActionActive("CameraDown")) {
            movement = Subtract(movement, camera.up);
        }

        if (Length(movement) > 0.0001f) {
            camera.position = Add(camera.position, Scale(Normalize(movement), frameSpeed));
        }

        if (input.IsActionActive("CameraLook")) {
            const Vec2 mouseDelta = input.GetMouseDelta();
            camera.yaw += mouseDelta.x * mouseSensitivity;
            camera.pitch -= mouseDelta.y * mouseSensitivity;
            camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
            camera.forward = ForwardFromYawPitch(camera.yaw, camera.pitch);
        }

        renderer.SetCamera(camera);
        return;
    }
}
