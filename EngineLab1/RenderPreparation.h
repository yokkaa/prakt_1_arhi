#pragma once

#include "BenchmarkRecorder.h"
#include "Components.h"
#include "JobSystem.h"
#include "PreparedModelMatrix.h"

#include "raylib.h"
#include "raymath.h"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <future>
#include <vector>

inline PreparedModelMatrix BuildModelMatrix(
    const TransformComponent& transform
) {
    const Matrix scale = MatrixScale(
        transform.scale.x,
        transform.scale.y,
        transform.scale.z
    );

    const Matrix rotationX = MatrixRotateX(
        transform.rotation.x * DEG2RAD
    );

    const Matrix rotationY = MatrixRotateY(
        transform.rotation.y * DEG2RAD
    );

    const Matrix rotationZ = MatrixRotateZ(
        transform.rotation.z * DEG2RAD
    );

    const Matrix rotation = MatrixMultiply(
        MatrixMultiply(rotationX, rotationY),
        rotationZ
    );

    const Matrix translation = MatrixTranslate(
        transform.position.x,
        transform.position.y,
        transform.position.z
    );

    const Matrix model = MatrixMultiply(
        MatrixMultiply(scale, rotation),
        translation
    );

    PreparedModelMatrix result{};

    result.values = {
        model.m0,  model.m1,  model.m2,  model.m3,
        model.m4,  model.m5,  model.m6,  model.m7,
        model.m8,  model.m9,  model.m10, model.m11,
        model.m12, model.m13, model.m14, model.m15
    };

    return result;
}

inline Matrix ToRaylibMatrix(
    const PreparedModelMatrix& prepared
) {
    const auto& values = prepared.values;

    return Matrix{
        values[0], values[4], values[8],  values[12],
        values[1], values[5], values[9],  values[13],
        values[2], values[6], values[10], values[14],
        values[3], values[7], values[11], values[15]
    };
}

inline std::size_t PrepareModelMatrices(
    const std::vector<TransformComponent>& snapshot,
    std::vector<PreparedModelMatrix>& output,
    JobSystem& jobs,
    bool parallel
) {
    ZoneScopedN("RenderPreparation::Total");

    using Clock = std::chrono::steady_clock;

    const auto started = Clock::now();

    const auto recordTime = [&]() {
        const auto finished = Clock::now();

        const double elapsedMs =
            std::chrono::duration<double, std::milli>(
                finished - started
            ).count();

        BenchmarkRecorder::Get().SetPreparationTime(elapsedMs);
    };

    output.resize(snapshot.size());

    if (snapshot.empty()) {
        recordTime();
        return 0;
    }

    // Каждый поток записывает только свой диапазон.
    // Размеры векторов во время выполнения задач не меняются.
    const auto calculateRange = [&snapshot, &output](
        std::size_t first,
        std::size_t last
    ) {
        ZoneScopedN("RenderPreparation::Batch");

        for (std::size_t index = first; index < last; ++index) {
            output[index] = BuildModelMatrix(snapshot[index]);
        }
    };

    if (!parallel || jobs.GetWorkerCount() == 0) {
        {
            ZoneScopedN("RenderPreparation::Sequential");

            calculateRange(0, snapshot.size());
        }

        recordTime();
        return 0;
    }

    const std::size_t taskLimit = (std::min)(
        snapshot.size(),
        static_cast<std::size_t>(jobs.GetWorkerCount())
    );

    const std::size_t batchSize =
        (snapshot.size() + taskLimit - 1) / taskLimit;

    std::size_t submittedJobs = 0;

    {
        std::vector<std::future<void>> pending;
        pending.reserve(taskLimit);

        std::exception_ptr firstError;

        {
            ZoneScopedN("RenderPreparation::Dispatch");

            try {
                for (
                    std::size_t first = 0;
                    first < snapshot.size();
                    first += batchSize
                ) {
                    const std::size_t last = (std::min)(
                        first + batchSize,
                        snapshot.size()
                    );

                    pending.push_back(
                        jobs.Submit(
                            [calculateRange, first, last]() {
                                calculateRange(first, last);
                            }
                        )
                    );
                }
            }
            catch (...) {
                firstError = std::current_exception();
            }
        }

        // Даже при ошибке ждём все уже отправленные задачи:
        // они используют ссылки на snapshot и output.
        {
            ZoneScopedN("RenderPreparation::Wait");

            for (auto& future : pending) {
                try {
                    future.get();
                }
                catch (...) {
                    if (!firstError) {
                        firstError = std::current_exception();
                    }
                }
            }
        }

        if (firstError) {
            std::rethrow_exception(firstError);
        }

        submittedJobs = pending.size();
    }

    recordTime();

    return submittedJobs;
}