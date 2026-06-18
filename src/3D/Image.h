#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

/*
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

struct Image
{
public:
    std::string uri;
    Uint32 width;
    Uint32 height;
    Uint32 component;
    Uint32 layers = 1;
    SDL_GPUTextureType type = SDL_GPU_TEXTURETYPE_2D;
    std::vector<Uint32> pixels;
    Uint32 num_levels = 1;
    SDL_GPUTexture* texture;

    Uint32 total_byte_count() { return pixels.size(); }

    void Prepare(SDL_GPUDevice* device);
    void Stage(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transferBuffer,
        Uint32 offset = 0, Uint32 limit = UINT32_MAX);
    void StageLayer(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transferBuffer,
        Uint32 layer, Uint32 offset = 0);
    void Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass,
        SDL_GPUTransferBuffer* transferBuffer, Uint32 offset = 0, Uint32 layer = 0);
    void Release(SDL_GPUDevice* device);

    void GenerateMipmaps(SDL_GPUCommandBuffer* cmd);
};
