#pragma once
#include <SDL3/SDL.h>

class Shader
{
private:
    SDL_GPUShader* shader;

public:
    void Load(SDL_GPUDevice* device,
        const char* shaderFilename);
    void Release(SDL_GPUDevice* device);
    SDL_GPUShader* GetShader();
};
