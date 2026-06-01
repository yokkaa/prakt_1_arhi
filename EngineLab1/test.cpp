#include <memory>
#include <string>
#include <iostream>

struct TestData {
    std::string value;
};

std::shared_ptr<TestData> TestLoader(const std::string& path) {
    auto data = std::make_shared<TestData>();
    data->value = "Loaded from " + path;
    return data;
}