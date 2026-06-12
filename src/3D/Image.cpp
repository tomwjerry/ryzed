#include "Image.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

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

void Image::Stage(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transferBuffer,
    Uint32 offset, Uint32 limit)
{
    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(reinterpret_cast<Uint8*>(transferData) + offset, this->pixels.data(),
        this->pixels.size());
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);
}

void Image::StageLayer(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transferBuffer,
    Uint32 layer, Uint32 offset)
{
    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    
    // 1. Calculate the number of elements per layer
    size_t elementsPerLayer = static_cast<size_t>(this->width) * this->height;
    
    // 3. Pointer math: Start of the layer in the vector
    // pixels.data() returns Uint64*, so the offset should be in elements, not bytes
    const Uint64* sourcePtr = this->pixels.data() + (layer * elementsPerLayer);
    
    // 4. Copy the data
    SDL_memcpy(reinterpret_cast<Uint8*>(transferData) + offset, sourcePtr, elementsPerLayer);
    
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);
}

void Image::Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass,
    SDL_GPUTransferBuffer* transferBuffer, Uint32 offset, Uint32 layer)
{
    SDL_GPUTextureTransferInfo src = {
        transferBuffer,
        offset
    };
    SDL_GPUTextureRegion dst = SDL_GPUTextureRegion();
    dst.texture = this->texture;
    dst.h = static_cast<Uint32>(this->height);
    dst.w = static_cast<Uint32>(this->width);
    dst.layer = layer;
    dst.d = 1;
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
