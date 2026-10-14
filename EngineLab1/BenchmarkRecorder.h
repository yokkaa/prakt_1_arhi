#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

// Все методы вызываются только из главного потока.
class BenchmarkRecorder {
public:
    static BenchmarkRecorder& Get() {
        static BenchmarkRecorder recorder;
        return recorder;
    }

    void Start(bool parallel) {
        samples.clear();
        samples.reserve(100000);

        mode = parallel ? "jobs" : "sequential";

        warmupElapsedMs = 0.0;
        recordingElapsedMs = 0.0;
        preparationMs = 0.0;
        preparationAvailable = false;

        warmingUp = true;
        running = true;

        std::cout
            << "Benchmark started: " << mode
            << " | warmup: 5 seconds"
            << " | recording: 10 seconds"
            << std::endl;
    }

    void Cancel() {
        if (!running) {
            return;
        }

        running = false;
        warmingUp = false;
        samples.clear();

        std::cout << "Benchmark cancelled" << std::endl;
    }

    bool IsRunning() const {
        return running;
    }

    bool IsWarmingUp() const {
        return running && warmingUp;
    }

    double GetRemainingSeconds() const {
        if (!running) {
            return 0.0;
        }

        const double remainingMs = warmingUp
            ? WarmupDurationMs - warmupElapsedMs
            : RecordingDurationMs - recordingElapsedMs;

        return (std::max)(0.0, remainingMs / 1000.0);
    }

    // Вызывается в начале каждого кадра.
    void BeginFrame() {
        preparationAvailable = false;
        preparationMs = 0.0;
    }

    // Получает время полного расчёта матриц:
    // включая отправку задач и ожидание их завершения.
    void SetPreparationTime(double milliseconds) {
        if (
            !std::isfinite(milliseconds) ||
            milliseconds < 0.0
        ) {
            return;
        }

        preparationMs = milliseconds;
        preparationAvailable = true;
    }

    // Вызывается после завершения кадра.
    // Запись на диск начинается только после окончания замера.
    void RecordFrame(double frameMs) {
        if (
            !running ||
            !preparationAvailable ||
            !std::isfinite(frameMs) ||
            frameMs <= 0.0
        ) {
            return;
        }

        if (warmingUp) {
            warmupElapsedMs += frameMs;

            if (warmupElapsedMs >= WarmupDurationMs) {
                warmingUp = false;
                recordingElapsedMs = 0.0;
            }

            // Последний кадр прогрева тоже не включаем в замер.
            return;
        }

        samples.push_back({
            frameMs,
            preparationMs
        });

        recordingElapsedMs += frameMs;

        if (recordingElapsedMs >= RecordingDurationMs) {
            running = false;

            try {
                SaveResults();
            }
            catch (const std::exception& error) {
                std::cerr
                    << "Cannot save benchmark: "
                    << error.what()
                    << std::endl;
            }
        }
    }

private:
    struct Sample {
        double frameMs;
        double preparationMs;
    };

    static constexpr double WarmupDurationMs = 5000.0;
    static constexpr double RecordingDurationMs = 10000.0;

    bool running = false;
    bool warmingUp = false;
    bool preparationAvailable = false;

    double warmupElapsedMs = 0.0;
    double recordingElapsedMs = 0.0;
    double preparationMs = 0.0;

    std::string mode;
    std::vector<Sample> samples;

    BenchmarkRecorder() = default;

    // Метод ближайшего ранга для p95 и p99.
    static double Percentile(
        const std::vector<double>& sorted,
        double fraction
    ) {
        if (sorted.empty()) {
            return 0.0;
        }

        const std::size_t rank = static_cast<std::size_t>(
            std::ceil(
                fraction * static_cast<double>(sorted.size())
            )
        );

        const std::size_t index = rank > 0 ? rank - 1 : 0;

        return sorted[
            (std::min)(index, sorted.size() - 1)
        ];
    }

    static double Median(const std::vector<double>& sorted) {
        if (sorted.empty()) {
            return 0.0;
        }

        const std::size_t middle = sorted.size() / 2;

        if (sorted.size() % 2 == 0) {
            return (sorted[middle - 1] + sorted[middle]) * 0.5;
        }

        return sorted[middle];
    }

    void SaveResults() const {
        namespace fs = std::filesystem;

        const fs::path root = "BenchmarkResults";
        fs::create_directories(root);

        const auto timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();

        const std::string baseName =
            mode + "_" + std::to_string(timestamp);

        fs::path runDirectory = root / baseName;

        // Не перезаписываем предыдущий прогон
        // даже при совпадении временной метки.
        std::size_t suffix = 0;

        while (!fs::create_directory(runDirectory)) {
            ++suffix;
            runDirectory = root / (
                baseName + "_" + std::to_string(suffix)
            );
        }

        std::vector<double> frameTimes;
        std::vector<double> preparationTimes;

        frameTimes.reserve(samples.size());
        preparationTimes.reserve(samples.size());

        std::ofstream raw(runDirectory / "frames.csv");

        if (!raw) {
            throw std::runtime_error("Cannot open frames.csv");
        }

        raw.imbue(std::locale::classic());
        raw << std::fixed << std::setprecision(6);
        raw << "frame_index,frame_ms,preparation_ms\n";

        for (std::size_t i = 0; i < samples.size(); ++i) {
            const Sample& sample = samples[i];

            raw
                << i << ','
                << sample.frameMs << ','
                << sample.preparationMs << '\n';

            frameTimes.push_back(sample.frameMs);
            preparationTimes.push_back(sample.preparationMs);
        }

        raw.close();

        if (!raw) {
            throw std::runtime_error("Cannot finish frames.csv");
        }

        std::sort(frameTimes.begin(), frameTimes.end());
        std::sort(preparationTimes.begin(), preparationTimes.end());

        std::ofstream summary(runDirectory / "summary.csv");

        if (!summary) {
            throw std::runtime_error("Cannot open summary.csv");
        }

        summary.imbue(std::locale::classic());
        summary << std::fixed << std::setprecision(6);

        summary
            << "mode,metric,samples,median_ms,p95_ms,p99_ms\n";

        const auto writeMetric = [&](
            const char* name,
            const std::vector<double>& values
        ) {
            summary
                << mode << ','
                << name << ','
                << values.size() << ','
                << Median(values) << ','
                << Percentile(values, 0.95) << ','
                << Percentile(values, 0.99) << '\n';
        };

        writeMetric("frame", frameTimes);
        writeMetric("preparation", preparationTimes);

        summary.close();

        if (!summary) {
            throw std::runtime_error("Cannot finish summary.csv");
        }

        std::cout
            << "Benchmark saved: "
            << fs::absolute(runDirectory).string()
            << " | frames: " << samples.size()
            << std::endl;
    }
};