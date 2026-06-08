#include "Renderer.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen, and
 * Copyright (C) 2024 Caleb Cornett <caleb.cornett@outlook.com> (zlib Licence)
 */

Renderer::Renderer(SDL_GPUDevice* device, SDL_Window* window)
{
    this->device = device;
    this->window = window;
}

void Renderer::Init()
{
    int windowWidth;
    int windowHeight;
    SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);

    this->camera = new Camera(Vector3(0.0f, 0.5f, -5.0f),
        Vector3(0.0f, 0.5f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Math::Radians(60.0f),
        windowWidth / (float)windowHeight,
        0.1f,
        500.0f);

    SDL_GPUTextureCreateInfo msaaTextureCreateInfo = SDL_GPUTextureCreateInfo();
    msaaTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    msaaTextureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    msaaTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    msaaTextureCreateInfo.width = static_cast<uint32_t>(windowWidth);
    msaaTextureCreateInfo.height = static_cast<uint32_t>(windowHeight);
    msaaTextureCreateInfo.layer_count_or_depth = 1;
    msaaTextureCreateInfo.num_levels = 1;
    msaaTextureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_4;
    this->msaaTexture = SDL_CreateGPUTexture(device, &msaaTextureCreateInfo);

    SDL_GPUTextureCreateInfo depthTextureCreateInfo = SDL_GPUTextureCreateInfo();
    depthTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    depthTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthTextureCreateInfo.width = static_cast<uint32_t>(windowWidth);
    depthTextureCreateInfo.height = static_cast<uint32_t>(windowHeight);
    depthTextureCreateInfo.layer_count_or_depth = 1;
    depthTextureCreateInfo.num_levels = 1;
    depthTextureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // Must match color target sample count
    this->depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);

    SDL_GPUTextureCreateInfo resolveTextureCreateInfo = SDL_GPUTextureCreateInfo();
    resolveTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    resolveTextureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    resolveTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    resolveTextureCreateInfo.width = static_cast<uint32_t>(windowWidth);
    resolveTextureCreateInfo.height = static_cast<uint32_t>(windowHeight);
    resolveTextureCreateInfo.layer_count_or_depth = 1;
    resolveTextureCreateInfo.num_levels = 1;
    this->resolveTexture = SDL_CreateGPUTexture(device, &resolveTextureCreateInfo);
}

void Renderer::AddRenderable(IRenderable* renderable)
{
    this->renderables.push_back(renderable);
}

void Renderer::Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* renderPass)
{
    CameraInfo camInfo;
    camInfo.view = this->camera->GetViewMatrix();
    camInfo.proj = this->camera->GetProjMatrix();
    Vector3 cameraPos = this->camera->GetEye();

    SDL_PushGPUVertexUniformData(cmd, 0, &camInfo, sizeof(CameraInfo));
    //SDL_PushGPUFragmentUniformData(cmd, 0, &cameraPos, sizeof(float[3])); // For lightning

    for (const auto renderable : this->renderables)
    {
        renderable->Render(renderPass, cmd);
    }
}
