#include "Shader.h"
#include <SDL3_shadercross/SDL_shadercross.h>

void Shader::Load(
    SDL_GPUDevice* device,
    const char* shaderFilename,
    Uint32 samplerCount,
    Uint32 uniformBufferCount,
    Uint32 storageBufferCount,
    Uint32 storageTextureCount
)
{
    SDL_ShaderCross_ShaderStage stage;

    // Auto-detect the shader stage from the file name for convenience
    if (SDL_strstr(shaderFilename, ".vert"))
    {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(shaderFilename, ".frag"))
    {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        SDL_Log("Invalid shader stage!");
        return;
    }

    char fullPath[256];
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint = "main";
    SDL_snprintf(fullPath, sizeof(fullPath), "%sresources/Shaders/%s.spv",
        BasePath, shaderFilename);

    size_t codeSize;
    void* code = SDL_LoadFile(fullPath, &codeSize);
    if (code == NULL)
    {
        SDL_Log("Failed to load shader from disk! %s", fullPath);
        return;
    }

    SDL_ShaderCross_SPIRV_Info shaderInfo = {
        code,
        codeSize,
        entrypoint,
        stage,
        0
    };

    SDL_ShaderCross_GraphicsShaderMetadata* mdata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        code,
        codeSize,
        0
    );

    this->shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        device,
        &this->shaderInfo,
        mdata,
        0
    );
    
    if (shader == NULL)
    {
        SDL_Log("Failed to create shader!");
    }
}

SDL_GPUShader* Shader::GetShader()
{
    return this->shader;
}

void Shader::Release(SDL_GPUDevice* device)
{
    SDL_ReleaseGPUShader(device, this->shader);
}
