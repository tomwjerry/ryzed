#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include <unordered_map>
#include "Node.h"
#include "Material.h"
#include "../2D/Image.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

class Scene
{
private:
    std::vector<Node*> nodes;
    std::unordered_map<std::string, Material*> materials;
    std::unordered_map<std::string, Image*> images;

    SDL_GPUBuffer* Scene::createBuffer(
        SDL_GPUDevice* device, const void* data, size_t size, SDL_GPUBufferUsageFlags usage)

public:
    void AddNode(Node* node);
    void PrepareForRender(SDL_GPUDevice* device);
    void Render(SDL_GPUDevice* device);
};
