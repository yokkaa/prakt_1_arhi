#pragma once

#include <array>

// Готовая матрица положения, поворота и масштаба объекта.
// Храним независимо от графической библиотеки.
// Элементы расположены по столбцам.
struct PreparedModelMatrix {
    std::array<float, 16> values{};
};