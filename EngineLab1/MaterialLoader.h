#pragma once

#include <memory>
#include <string>

#include "Material.h"

class MaterialLoader {
public:
    static std::shared_ptr<MaterialData> LoadMaterial(
        const std::string& path
    );
};
