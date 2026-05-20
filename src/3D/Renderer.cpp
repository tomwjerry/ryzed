#include "Renderer.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen, and
 * Copyright (C) 2024 Caleb Cornett <caleb.cornett@outlook.com> (zlib Licence)
 */

Renderer::Renderer(SDL_GPUDevice* device)
{
    this->device = device;
}

Renderer::Init()
{
    Shader* landscapeVert = new Shader();
    landscapeVert->Load("General.vert");
    Shader* landscapeFrag = new Shader();
    landscapeFrag->Load("General.frag");

    // Set up pipeline
    SDL_GPUTextureFormat colorTargetFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    SDL_GPUTextureFormat depthTargetFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    SDL_GPUTextureFormat swapchainImageFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    SDL_GPUSampleCount msaaSampleCount = SDL_GPU_SAMPLECOUNT_4;
    if (!SDL_GPUTextureSupportsSampleCount(device, colorTargetFormat, msaaSampleCount))
    {
		SDL_Log("Sample count %d is not supported", (1 << static_cast<int>(msaaSampleCount)));
        msaaSampleCount = SDL_GPU_SAMPLECOUNT_4;
	}

    SDL_GPUVertexBufferDescription landscapeVBD[2] = {{
        0,
        sizeof(float[3]),
        SDL_GPU_VERTEXINPUTRATE_VERTEX,
        0
    },
    {
        1,
        sizeof(float[2]),
        SDL_GPU_VERTEXINPUTRATE_VERTEX,
        0
    }};

    SDL_GPUVertexAttributeDescription landscapeVA[2] = {{
        0,
        0,
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        0
    },
    {
        1,
        0,
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        0
    }};

    SDL_GPUGraphicsPipelineCreateInfo landscapePipelineInfo;
    landscapePipelineInfo.vertex_shader = landscapeVert->Get();
	landscapePipelineInfo.fragment_shader = landscapeFrag->Get();
    
    landscapePipelineInfo.vertex_input_state = SDL_GPUVertexInputState();
    landscapePipelineInfo.vertex_input_state.vertex_buffer_descriptions = landscapeVBD;
    landscapePipelineInfo.vertex_input_state.num_vertex_buffers = 2;
    landscapePipelineInfo.vertex_input_state.vertex_attributes = landscapeVA;
    landscapePipelineInfo.vertex_input_state.num_vertex_attributes = 2;

    landscapePipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    landscapePipelineInfo.rasterizer_state = SDL_GPURasterizerState();
    landscapePipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    landscapePipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
    landscapePipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    landscapePipelineInfo.multisample_state = SDL_GPUMultisampleState();
    landscapePipelineInfo.multisample_state.sample_count = msaaSampleCount;
    
    landscapePipelineInfo.depth_stencil_state = SDL_GPUDepthStencilState();
    landscapePipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    landscapePipelineInfo.depth_stencil_state.depth_write_enable = true;
    landscapePipelineInfo.depth_stencil_state.stencil_write_enable = true;
    
    landscapePipelineInfo.target_info = SDL_GPUGraphicsPipelineTargetInfo();
    landscapePipelineInfo.target_info.color_target_descriptions = SDL_GPUColorTargetDescription[1];
    landscapePipelineInfo.target_info.color_target_descriptions[0].format = colorTargetFormat;
    landscapePipelineInfo.target_info.num_color_targets = 1;
    landscapePipelineInfo.target_info.depth_stencil_format = depthTargetFormat;
    landscapePipelineInfo.target_info.has_depth_stencil_target = true;

    SDL_GPUGraphicsPipeline* landscapePipeline =
        SDL_CreateGPUGraphicsPipeline(device, &landscapePipelineInfo);
	if (landscapePipeline == NULL)
    {
		SDL_Log("Failed to create textured fill pipeline!");
		return -1;
	}

    // Release shaders
    landscapeVert->Release(device);
    landscapeFrag->Release(device);
}
