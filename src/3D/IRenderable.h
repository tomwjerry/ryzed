#pragma once
#include <vector>
#include <SDL3/SDL.h>

class IRenderable
{
public:
    virtual ~IRenderable() {}
    virtual bool LoadShaders(SDL_Window* window) = 0;
    virtual bool PrepareRender() = 0;
    virtual bool Render(SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* cmd) = 0;
};
