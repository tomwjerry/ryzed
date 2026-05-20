#include "Scene.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

void Scene::AddNode(Node* node)
{
    nodes.push_back(node);
}

void Scene::PrepareForRender()
{

}

SDL_GPUBuffer* Scene::createBuffer(
    SDL_GPUDevice* device, const void* data, size_t size, SDL_GPUBufferUsageFlags usage)
{
    SDL_GPUBufferCreateInfo bufferCreateInfo = SDL_GPUBufferCreateInfo();
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.size = static_cast<Uint32>(size);
    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
    
    SDL_GPUTransferBufferCreateInfo transferInfo = SDL_GPUTransferBufferCreateInfo();
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(size);
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    memcpy(transferData, data, size);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = SDL_GPUTransferBufferLocation();
    src.transfer_buffer = transferBuffer;
    src.offset = 0;
    SDL_GPUBufferRegion dst = SDL_GPUBufferRegion();
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = static_cast<Uint32>(size);
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    return buffer;
}