#pragma once
#include <memory>
#include <string>

template <typename T>
class Resource {
public:
    Resource(const std::string& path, std::shared_ptr<T> data)
        : m_path(path), m_data(data) {
    }

    std::shared_ptr<T> Get() const { return m_data; }
    std::string GetPath() const { return m_path; }

private:
    std::string m_path;
    std::shared_ptr<T> m_data;
};