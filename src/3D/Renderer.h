#pragma once

#include "Scene.h"
#include "Shader.h"
#include <SDL3/SDL.h>

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen, and
 * Copyright (C) 2024 Caleb Cornett <caleb.cornett@outlook.com> (zlib Licence)
 */

class Renderer
{
private:
    SDL_GPUDevice* device;
public:
    Renderer(SDL_GPUDevice* device);
    void Init();
};
