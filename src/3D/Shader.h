#pragma once
#include <SDL3/SDL.h>

class Shader
{
private:
    SDL_GPUShader* shader;

public:
    void Load(SDL_GPUDevice* device,
        const char* shaderFilename,
        Uint32 samplerCount,
        Uint32 uniformBufferCount,
        Uint32 storageBufferCount,
        Uint32 storageTextureCount);
    void Release();
    SDL_GPUShader* GetShader(SDL_GPUDevice* device);
};
