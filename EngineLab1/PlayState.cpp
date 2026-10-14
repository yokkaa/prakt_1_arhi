#include "PlayState.h"

#include "Application.h"
#include "BenchmarkRecorder.h"
#include "BenchmarkScene.h"
#include "InputManager.h"
#include "Logger.h"
#include "RenderAdapter.h"
#include "SceneSerializer.h"
#include "World.h"

#include <tracy/Tracy.hpp>

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

void PlayState::OnEnter() {
    Logger::Info("Entered PlayState");

    resourceRenderer = nullptr;

    parallelRenderPreparation = true;
    asynchronousResourceLoading = true;
    benchmarkSceneEnabled = false;

    sceneRequested = false;
    sceneReady = false;
    sceneFailed = false;
    debugCollidersEnabled = false;
}

void PlayState::OnExit() {
    BenchmarkRecorder::Get().Cancel();

    if (resourceRenderer) {
        resourceManager.ReleaseAll(*resourceRenderer);
        resourceRenderer = nullptr;
    }

    Logger::Info("Exited PlayState");
}

void PlayState::RequestScene(Application& app, bool benchmark) {
    ZoneScopedN("Scene::Request");

    BenchmarkRecorder::Get().Cancel();

    resourceRenderer = &app.GetRenderer();
    resourceManager.ReleaseAll(*resourceRenderer);
    resourceManager.SetJobSystem(app.GetJobs());
    resourceManager.SetAsynchronousLoading(
        asynchronousResourceLoading
    );

    benchmarkSceneEnabled = benchmark;

    sceneRequested = true;
    sceneReady = false;
    sceneFailed = false;
    debugCollidersEnabled = false;

    try {
        if (benchmark) {
            sceneFuture = app.GetJobs().Submit([]() -> World {
                ZoneScopedN("Scene::BuildBenchmark");
                return BenchmarkScene::Create();
            });

            Logger::Info("Benchmark scene queued in JobSystem");
        }
        else {
            const std::string path = "Assets/Scenes/demo_scene.json";

            sceneFuture = app.GetJobs().Submit([path]() -> World {
                ZoneScopedN("Scene::LoadCPU");

                World loadedWorld;

                if (!SceneSerializer::LoadSceneCPU(path, loadedWorld)) {
                    throw std::runtime_error(
                        "Cannot load scene: " + path
                    );
                }

                return loadedWorld;
            });

            Logger::Info("Demo scene queued in JobSystem");
        }
    }
    catch (const std::exception& error) {
        sceneFailed = true;

        Logger::Error(
            std::string("Cannot request scene: ") + error.what()
        );
    }
    catch (...) {
        sceneFailed = true;
        Logger::Error("Cannot request scene: unknown exception");
    }
}

void PlayState::Update(float deltaTime, Application& app) {
    resourceRenderer = &app.GetRenderer();
    resourceManager.SetJobSystem(app.GetJobs());

    InputManager& input = app.GetInput();
    BenchmarkRecorder& recorder = BenchmarkRecorder::Get();

    if (input.IsActionPressed("BackToMenu")) {
        recorder.Cancel();
        app.ChangeStateToMenu();
        return;
    }

    if (!sceneRequested) {
        RequestScene(app, false);
        return;
    }

    // Не меняем сцену во время записи.
    if (!recorder.IsRunning() &&
        (sceneReady || sceneFailed) &&
        input.IsKeyPressed(KeyCode::KEY_B)) {
        RequestScene(app, !benchmarkSceneEnabled);
        return;
    }
// Меняем режим только после завершения текущей загрузки.
// Во время записи benchmark переключение запрещено.
    if (!recorder.IsRunning() &&
        (sceneReady || sceneFailed) &&
        !resourceManager.HasPendingLoads()) {

        if (input.IsKeyPressed(KeyCode::KEY_L)) {
            asynchronousResourceLoading =
                !asynchronousResourceLoading;

            if (asynchronousResourceLoading) {
                Logger::Info("Resource loading: ASYNC");
                TracyMessageL("Resource loading: ASYNC");
            } 
            else {
            Logger::Info("Resource loading: SYNC");
            TracyMessageL("Resource loading: SYNC");
            }

            RequestScene(app, benchmarkSceneEnabled);
            return;
        }

        if (input.IsKeyPressed(KeyCode::KEY_O)) {
        TracyMessageL("Resource loading: RELOAD");
        RequestScene(app, benchmarkSceneEnabled);
        return;
        }
    }
    if (sceneFailed) {
        return;
    }

    if (!sceneReady) {
        const auto status = sceneFuture.wait_for(
            std::chrono::seconds(0)
        );

        if (status != std::future_status::ready) {
            return;
        }

        try {
            ZoneScopedN("Scene::Apply");

            World loadedWorld = sceneFuture.get();

            bool hasCamera = false;

            for (Entity entity : loadedWorld.GetEntities()) {
                if (loadedWorld.HasCamera(entity)) {
                    hasCamera = true;
                    break;
                }
            }

            if (!hasCamera) {
                const Entity camera = loadedWorld.CreateEntity();

                loadedWorld.AddTag(
                    camera,
                    Tag{ "Main Camera" }
                );

                loadedWorld.AddCamera(
                    camera,
                    CameraComponent{}
                );
            }

            app.GetWorld() = std::move(loadedWorld);

            if (benchmarkSceneEnabled) {
                // В измерительной сцене камера неподвижна.
                for (Entity entity : app.GetWorld().GetEntities()) {
                    if (!app.GetWorld().HasCamera(entity)) {
                        continue;
                    }

                    const CameraComponent& camera =
                        app.GetWorld().GetCamera(entity);

                    if (camera.active) {
                        app.GetRenderer().SetCamera(camera);
                        break;
                    }
                }
            }
            else {
                cameraSystem.Update(
                    app.GetWorld(),
                    input,
                    app.GetRenderer(),
                    0.0f
                );
            }

            sceneReady = true;

            Logger::Info(
                benchmarkSceneEnabled
                    ? "Benchmark scene applied: 10000 objects"
                    : "Demo scene applied on main thread"
            );
        }
        catch (const std::exception& error) {
            sceneFailed = true;

            Logger::Error(
                std::string("Scene loading failed: ") + error.what()
            );
        }
        catch (...) {
            sceneFailed = true;
            Logger::Error("Scene loading failed: unknown exception");
        }

        return;
    }

    // Один замер должен целиком относиться к одному режиму.
    if (!recorder.IsRunning() &&
        input.IsKeyPressed(KeyCode::KEY_P)) {
        parallelRenderPreparation = !parallelRenderPreparation;

        if (parallelRenderPreparation) {
            Logger::Info("Render preparation: JOB SYSTEM");
            TracyMessageL("Render mode: JOBS");
        }
        else {
            Logger::Info("Render preparation: SEQUENTIAL");
            TracyMessageL("Render mode: SEQUENTIAL");
        }
    }

    if (benchmarkSceneEnabled) {
        const bool resourcesReady =
            !resourceManager.HasPendingLoads() &&
            renderSystem.GetObjectCount() ==
                BenchmarkScene::ObjectCount;

        if (recorder.IsRunning() && !resourcesReady) {
            recorder.Cancel();

            Logger::Error(
                "Benchmark cancelled: scene resources are not ready"
            );
        }

        if (input.IsKeyPressed(KeyCode::KEY_R) &&
            !recorder.IsRunning()) {
            if (resourcesReady) {
                recorder.Start(parallelRenderPreparation);

                if (parallelRenderPreparation) {
                    TracyMessageL("Benchmark warmup started: JOBS");
                }
                else {
                    TracyMessageL("Benchmark warmup started: SEQUENTIAL");
                }
            }
            else {
                Logger::Info(
                    "Wait until all 10000 benchmark objects are loaded, "
                    "then press R again"
                );
            }
        }

        // Никакого движения камеры и физики в измерительной сцене.
        return;
    }

    World& world = app.GetWorld();

    if (input.IsActionPressed("SaveScene")) {
        SceneSerializer::SaveScene(
            "Assets/Scenes/saved_scene.json",
            world
        );
    }

    if (input.IsActionPressed("ToggleDebugColliders")) {
        debugCollidersEnabled = !debugCollidersEnabled;
    }

    playerControlSystem.Update(world, input, deltaTime);

    physicsSystem.Update(
        world,
        deltaTime,
        &app.GetEventBus()
    );

    cameraSystem.Update(
        world,
        input,
        app.GetRenderer(),
        deltaTime
    );
}

void PlayState::Render(RenderAdapter& renderer, Application& app) {
    if (sceneFailed) {
        renderer.DrawTextOnScreen(
            "SCENE LOAD FAILED - see terminal",
            35, 240, 25
        );

        renderer.DrawTextOnScreen(
            "B - other scene | ESC - menu",
            35, 280, 22
        );

        return;
    }

    if (!sceneReady) {
        renderer.DrawTextOnScreen(
            benchmarkSceneEnabled
                ? "BUILDING BENCHMARK..."
                : "LOADING DEMO SCENE...",
            100, 250, 25
        );

        renderer.DrawTextOnScreen(
            "ESC - menu",
            100, 290, 20
        );

        return;
    }

    {
        ZoneScopedN("Resources::Pump");
        resourceManager.PumpUploads(renderer);
    }

    // В benchmark не проверяем изменения файлов шейдеров:
    // условия остаются одинаковыми для обоих режимов.
    if (!benchmarkSceneEnabled) {
        ZoneScopedN("Resources::ShaderHotReload");
        resourceManager.ReloadChangedShaders(renderer);
    }

    renderer.Begin3D();

    {
        ZoneScopedN("RenderSystem::Update");

        renderSystem.Update(
            app.GetWorld(),
            renderer,
            resourceManager,
            app.GetDeltaTime(),
            app.GetJobs(),
            parallelRenderPreparation
        );
    }

    if (!benchmarkSceneEnabled && debugCollidersEnabled) {
        debugRenderSystem.RenderColliders(
            app.GetWorld(),
            renderer
        );
    }

    renderer.End3D();

    BenchmarkRecorder& recorder = BenchmarkRecorder::Get();

    const bool benchmarkReady =
        benchmarkSceneEnabled &&
        !resourceManager.HasPendingLoads() &&
        renderSystem.GetObjectCount() == BenchmarkScene::ObjectCount;

    // Проверяем и текущий кадр перед передачей его в recorder.
    if (recorder.IsRunning() && !benchmarkReady) {
        recorder.Cancel();

        Logger::Error(
            "Benchmark cancelled: incomplete render frame"
        );
    }

    if (benchmarkSceneEnabled) {
        renderer.DrawTextOnScreen(
            "BENCHMARK: 10000 OBJECTS",
            20, 20, 25
        );

        renderer.DrawTextOnScreen(
            "Fixed camera | B - demo | P - mode",
            20, 55, 20
        );

        if (recorder.IsRunning()) {
            const int seconds = static_cast<int>(
                std::ceil(recorder.GetRemainingSeconds())
            );

            const std::string status =
                std::string(
                    recorder.IsWarmingUp()
                        ? "WARMUP: "
                        : "RECORDING: "
                ) +
                std::to_string(seconds) +
                " s";

            renderer.DrawTextOnScreen(
                status,
                20, 90, 25
            );

            renderer.DrawTextOnScreen(
                "P / B locked | ESC - cancel and leave",
                20, 125, 20
            );
        }
        else if (benchmarkReady) {
            renderer.DrawTextOnScreen(
                "R - record: 5 s warmup + 10 s measurement",
                20, 90, 20
            );

            renderer.DrawTextOnScreen(
                "CSV save path appears in terminal",
                20, 120, 20
            );
        }
        else {
            renderer.DrawTextOnScreen(
                "Waiting for benchmark resources...",
                20, 90, 20
            );
        }
    }
    else {
        renderer.DrawTextOnScreen(
            "DEMO: PHYSICS + INPUT + DEBUG",
            20, 20, 25
        );

        renderer.DrawTextOnScreen(
            "WASD + Space - player",
            20, 55, 20
        );

        renderer.DrawTextOnScreen(
            "Arrows/QE - camera | RMB - look | Shift - faster",
            20, 85, 20
        );

        renderer.DrawTextOnScreen(
            "F3 - colliders | F5 - save | B - benchmark",
            20, 115, 20
        );
    }
    renderer.DrawTextOnScreen(
    std::string("L - resource loading: ") +
        (asynchronousResourceLoading ? "ASYNC" : "SYNC") +
        " | O - reload",
    20, 450, 20
    );
    
    std::string mode = "SEQUENTIAL";

    if (parallelRenderPreparation) {
        mode = resourceManager.HasPendingLoads()
            ? "JOBS (loading: CPU)"
            : "JOBS";
    }

    renderer.DrawTextOnScreen(
        "P - preparation mode: " + mode,
        20, 490, 20
    );

    renderer.DrawTextOnScreen(
        "Objects: " + std::to_string(renderSystem.GetObjectCount()) +
        " | Jobs: " + std::to_string(renderSystem.GetJobCount()),
        20, 520, 20
    );

    renderer.DrawTextOnScreen(
        recorder.IsRunning()
            ? "ESC - cancel and return to menu"
            : "ESC - menu",
        20, 550, 20
    );

    if (!benchmarkSceneEnabled && debugCollidersEnabled) {
        renderer.DrawTextOnScreen(
            "DEBUG COLLIDERS: ON",
            350, 550, 20
        );
    }
}