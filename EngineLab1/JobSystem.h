#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class JobSystem {
public:
    explicit JobSystem(std::size_t workerCount = 0) {
        if (workerCount == 0) {
            const unsigned int hardwareThreads =
                std::thread::hardware_concurrency();

            workerCount = hardwareThreads > 1
                ? hardwareThreads - 1
                : 1;

            // Пока ограничиваем пул восемью рабочими потоками.
            if (workerCount > 8) {
                workerCount = 8;
            }
        }

        try {
            for (std::size_t i = 0; i < workerCount; ++i) {
                workers.emplace_back([this]() {
                    WorkerLoop();
                });
            }
        }
        catch (...) {
            Shutdown();
            throw;
        }
    }

    ~JobSystem() {
        Shutdown();
    }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    template <typename Function>
    auto Submit(Function&& function)
        -> std::future<std::invoke_result_t<std::decay_t<Function>&>> {

        using Result =
            std::invoke_result_t<std::decay_t<Function>&>;

        auto task = std::make_shared<std::packaged_task<Result()>>(
            std::forward<Function>(function)
        );

        auto result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (stopping) {
                throw std::runtime_error(
                    "Cannot submit a job after shutdown"
                );
            }

            jobs.emplace([task]() {
                (*task)();
            });
        }

        wakeCondition.notify_one();
        return result;
    }

    // Вызывать из главного потока.
    // Выполняет уже принятые задачи и присоединяет рабочие потоки.
    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopping = true;
        }

        wakeCondition.notify_all();

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        workers.clear();
    }

    // Вызывать из главного потока, вне Shutdown().
    std::size_t GetWorkerCount() const {
        return workers.size();
    }

private:
    void WorkerLoop() {
        while (true) {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> lock(queueMutex);

                wakeCondition.wait(lock, [this]() {
                    return stopping || !jobs.empty();
                });

                if (stopping && jobs.empty()) {
                    return;
                }

                job = std::move(jobs.front());
                jobs.pop();
            }

            // Выполняем вне блокировки: остальные потоки
            // могут одновременно забирать другие задачи.
            // packaged_task сохраняет исключения в future.
            job();
        }
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;

    std::mutex queueMutex;
    std::condition_variable wakeCondition;
    bool stopping = false;
};