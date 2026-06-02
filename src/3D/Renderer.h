#pragma once

#include "Camera.h"
#include "IRenderable.h"
#include "Shader.h"
#include <SDL3/SDL.h>
#include <vector>

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen, and
 * Copyright (C) 2024 Caleb Cornett <caleb.cornett@outlook.com> (zlib Licence)
 */

struct MVP {
    Mat4 view;
    Mat4 proj;
    Mat4 model;
};

class Renderer
{
private:
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPUTexture* msaaTexture = nullptr;
    SDL_GPUTexture* depthTexture = nullptr;
    SDL_GPUTexture* resolveTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    Camera* camera;

    std::vector<IRenderable*> renderables;

public:
    Renderer(SDL_GPUDevice* device, SDL_Window* window);
    void Init();
    void AddRenderable(IRenderable* renderable);
    void Render();
};
