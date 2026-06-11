#include "Image.h"
#include <SDL3/SDL.h>
#include <cmath>

/*
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

void Image::Prepare(SDL_GPUDevice* device)
{
    this->num_levels = std::max(1u, static_cast<Uint32>(
        std::floor(std::log2(std::min(width, height)))));

    SDL_GPUTextureCreateInfo textureDesc = SDL_GPUTextureCreateInfo();
	textureDesc.type = this->type;
	textureDesc.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
	textureDesc.width = static_cast<Uint32>(width);
	textureDesc.height = static_cast<Uint32>(height);
	textureDesc.layer_count_or_depth = this->layers;
	textureDesc.num_levels = this->num_levels;
    s
    if (this->component == 1)
    {
        textureDesc.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    }
    else if (this->component == 2)
    {
        textureDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM;
    }
    else if (this->component == 4)
    {
        textureDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }
    else if (this->component == 8)
    {
        textureDesc.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM;
    }
    else
    {
        SDL_Log("Unknown component count: %d", component);
        return;
    }
    this->texture =
        SDL_CreateGPUTexture(device, &textureDesc);

    this->pixels.resize(width * height);
}

void Image::Stage(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transferBuffer, Uint32 offset)
{
    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    memcpy(reinterpret_cast<Uint8*>(transferData) + offset, this->pixels.data(), this->pixels.size());
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);
}

void Image::Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass,
    SDL_GPUTransferBuffer* transferBuffer, Uint32 offset)
{
    SDL_GPUTextureTransferInfo src = {
        transferBuffer,
        offset
    };
    SDL_GPUTextureRegion dst = {
        this->texture,
        0,
        0,
        0,
        static_cast<Uint32>(this->width),
        static_cast<Uint32>(this->height),
        1
    };
    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
}

void Image::Release(SDL_GPUDevice* device)
{
    SDL_ReleaseGPUTexture(device, this->texture);
}

void Image::GenerateMipmaps(SDL_GPUCommandBuffer* cmd)
{
    if (this->texture == nullptr || this->num_levels <= 1)
    {
        return;
    }
    SDL_GenerateMipmapsForGPUTexture(cmd, this->texture);
}
