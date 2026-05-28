#pragma once
#include "psst/math/Vector.h"
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include "nel/ligo/primitive.h"

class RenderablePrimitive : public IPrimitive
{
private:
    std::string material;
    std::vector<float[2]> uvCoords;
    SDL_GPUBuffer* vbos[2];

    SDL_GPUBuffer* createBuffer(
        SDL_GPUDevice* device, const void* data, size_t size,
        SDL_GPUBufferUsageFlags usage);

public:
    void PrepareForRender(SDL_GPUDevice* device);
    void Render(SDL_GPUDevice* device);
};
