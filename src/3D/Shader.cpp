#include "Shader.h"
#include <SDL3_shadercross/SDL_shadercross.h>

void Shader::Load(
    SDL_GPUDevice* device,
    const char* shaderFilename
)
{
    SDL_ShaderCross_ShaderStage stage;

    // Auto-detect the shader stage from the file name for convenience
    if (SDL_strstr(shaderFilename, ".vert"))
    {
        stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(shaderFilename, ".frag"))
    {
        stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        SDL_Log("Invalid shader stage!");
        return;
    }

    char fullPath[256];
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* BasePath = SDL_GetBasePath();
    const char* entrypoint = "main";
    SDL_snprintf(fullPath, sizeof(fullPath), "%sresources/Shaders/%s.hlsl",
        BasePath, shaderFilename);

    size_t codeSize;
    void* code = SDL_LoadFile(fullPath, &codeSize);
    if (code == NULL)
    {
        SDL_Log("Failed to load shader from disk! %s", fullPath);
        return;
    }

    size_t spirvSize;
    SDL_ShaderCross_HLSL_Info hlsl_info;
    hlsl_info.source = (const char*)code;
    hlsl_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    hlsl_info.entrypoint = "main";
    void* spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
        &hlsl_info,
        &spirvSize
    );

    SDL_ShaderCross_SPIRV_Info shaderInfo = {
        (const Uint8*)spirv,
        spirvSize,
        entrypoint,
        stage,
        0
    };

    SDL_ShaderCross_GraphicsShaderMetadata* mdata = SDL_ShaderCross_ReflectGraphicsSPIRV(
        (const Uint8*)spirv,
        spirvSize,
        0
    );

    this->shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        device,
        &shaderInfo,
        &mdata->resource_info,
        0
    );
    
    if (shader == NULL)
    {
        SDL_Log("Failed to create shader!");
    }

    SDL_free(mdata);
}

SDL_GPUShader* Shader::GetShader()
{
    return this->shader;
}

void Shader::Release(SDL_GPUDevice* device)
{
    SDL_ReleaseGPUShader(device, this->shader);
}
