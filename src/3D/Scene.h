#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include <unordered_map>

#include "RenderablePrimitive.h"
#include "Material.h"
#include "../2D/Image.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

class Scene
{
private:
    std::vector<RenderablePrimitive*> nodes;
    std::unordered_map<std::string, Material*> materials;
    std::unordered_map<std::string, Image*> images;

public:
    void AddNode(RenderablePrimitive* node);
    void PrepareForRender(SDL_GPUDevice* device);
    void Render(SDL_GPUDevice* device);
};
