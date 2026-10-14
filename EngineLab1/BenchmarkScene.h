#pragma once

#include "World.h"
#include "Components.h"

namespace BenchmarkScene {

    inline constexpr int Columns = 100;
    inline constexpr int Rows = 100;
    inline constexpr int ObjectCount = Columns * Rows;

    inline World Create() {
        World world;

        // Камера смотрит на сетку объектов спереди.
        const Entity cameraEntity = world.CreateEntity();

        CameraComponent camera;
        camera.position = { 0.0f, 0.0f, 150.0f };
        camera.forward = { 0.0f, 0.0f, -1.0f };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.yaw = -90.0f;
        camera.pitch = 0.0f;
        camera.fov = 45.0f;
        camera.aspectRatio = 800.0f / 600.0f;
        camera.farPlane = 1000.0f;
        camera.active = true;

        world.AddTag(
            cameraEntity,
            Tag{ "Benchmark Camera" }
        );

        world.AddCamera(cameraEntity, camera);

        // Все объекты используют общую модель и материал.
        // Для каждого рассчитывается собственная матрица.
        MeshRendererComponent meshRenderer;
        meshRenderer.meshPath =
            "Assets/Benchmarks/benchmark_triangle.obj";

        meshRenderer.materialPath =
            "Assets/Materials/penguin_material.json";

        constexpr float spacing = 1.0f;

        const float centerX =
            static_cast<float>(Columns - 1) * 0.5f;

        const float centerY =
            static_cast<float>(Rows - 1) * 0.5f;

        for (int row = 0; row < Rows; ++row) {
            for (int column = 0; column < Columns; ++column) {
                const int index = row * Columns + column;

                const Entity entity = world.CreateEntity();

                TransformComponent transform;

                transform.position = {
                    (static_cast<float>(column) - centerX) * spacing,
                    (static_cast<float>(row) - centerY) * spacing,
                    0.0f
                };

                // Детерминированные повороты и масштаб.
                // Никаких случайных изменений между прогонами.
                transform.rotation = {
                    static_cast<float>((index % 7) - 3) * 3.0f,
                    static_cast<float>((index % 11) - 5) * 2.0f,
                    static_cast<float>(index % 12) * 15.0f
                };

                const float scale =
                    0.65f +
                    static_cast<float>(index % 5) * 0.05f;

                transform.scale = {
                    scale,
                    scale,
                    scale
                };

                world.AddTransform(entity, transform);
                world.AddMeshRenderer(entity, meshRenderer);
            }
        }

        return world;
    }

}