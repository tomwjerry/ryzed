/** 
 * AGPL(?), Based on the code by zerotacg
 */

#include "LandscapeManager.h"
#include "../3D/DummyDriver.h"
#include "../3D/Shader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "nel/3d/bezier_patch.h"
#include "nel/3d/nelu.h"
#include "nel/3d/texture_file.h"
#include "nel/3d/tile_bank.h"

#include "nel/ligo/primitive_utils.h"
#include "nel/ligo/zone_region.h"
#include "nel/misc/bitmap.h"
#include "nel/misc/cmd_args.h"
#include "nel/misc/common.h"
#include "nel/misc/config_file.h"
#include "nel/misc/file.h"
#include "nel/misc/i_xml.h"
#include "nel/misc/o_xml.h"
#include "nel/misc/path.h"
#include "nel/misc/types_nl.h"

LandscapeManager::LandscapeManager(SDL_GPUDevice* device, const std::string& cfgFile)
{
    this->tileBank = new NL3D::CTileBank();
    // Create a Landscape.
    DummyDriver* driver = new DummyDriver();
    /*NL3D::CNELU::Driver = driver;
    NL3D::CViewport viewport;
    NL3D::CNELU::initScene(viewport);
    NL3D::CNELU::Scene->setDriver(driver);
    this->landscapeModel =
        (NL3D::CLandscapeModel*)NL3D::CNELU::Scene->createModel(NL3D::LandscapeModelId);*/
    this->landscape = new NL3D::CLandscape();
    
    this->tileIdMaps[0] = new Image();
    this->tileIdMaps[1] = new Image();
    this->tileIdMaps[2] = new Image();

    this->device = device;

    this->worldTransform.m0 = 1.0f;
    this->worldTransform.m1 = 0.0f;
    this->worldTransform.m2 = 0.0f;
    this->worldTransform.m3 = 0.0f;

    this->worldTransform.m4 = 0.0f;
    this->worldTransform.m5 = 1.0f;
    this->worldTransform.m6 = 0.0f;
    this->worldTransform.m7 = 0.0f;

    this->worldTransform.m8 = 0.0f;
    this->worldTransform.m9 = 0.0f;
    this->worldTransform.m10 = 1.0f;
    this->worldTransform.m11 = 0.0f;

    this->worldTransform.m12 = 0.0f;
    this->worldTransform.m13 = 0.0f;
    this->worldTransform.m14 = 0.0f;
    this->worldTransform.m15 = 1.0f;

    this->worldTransform =
        Math::Translate(Vector3(-12490.0f, 9590.0f, -10.0f), this->worldTransform);

    NLMISC::CPath::addSearchPath("D:/ryzed/build/Debug");
    NLMISC::CPath::addSearchPath(
        "D:/ryzomcore-quickstart-4.0-pre3/leveldesign/primitives");

    this->tileBankFilePath =
        "D:/ryzomcore-quickstart-4.0-pre3/pipeline/install/jungle_bank/jungle.smallbank";
    /*this->ligoConfig = new NLLIGO::CLigoConfig();
    NLLIGO::CPrimitiveContext::instance().CurrentLigoConfig = this->ligoConfig;
    this->editLandscapePrimitive = new NLLIGO::CPrimitives();
    this->ligoConfig->readPrimitiveClass(cfgFile, false);
    NLLIGO::CPrimitiveContext::instance().CurrentPrimitive = this->editLandscapePrimitive;*/
}

LandscapeManager::~LandscapeManager()
{
    this->tileIdMaps[0]->Release(this->device);
    this->tileIdMaps[1]->Release(this->device);
    this->tileIdMaps[2]->Release(this->device);

    delete this->tileBank;
    delete this->tileIdMaps[0];
    delete this->tileIdMaps[1];
    delete this->tileIdMaps[2];
}

bool LandscapeManager::Load(const std::string& path)
{
    // A landscape is a .landscape.json file
    // Open the file
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    // "Parse" the JSON file (we don't even use a JSON parser, just pluck the strings)
    std::string parseText;

    while (std::getline(file, parseText))
    {
        if (parseText.find("zonesDir") != std::string::npos)
        {
            this->parsePath(parseText);
            this->LoadZoneDir(parseText);
        }
    }

    // Close the file
    file.close();

    return true;
}

bool LandscapeManager::LoadZoneDir(const std::string& path)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        this->LoadZone(entry.path().string());
    }

    return true;
}

bool LandscapeManager::LoadZone(const std::string& path)
{
    std::string zoneSearchDirectory = path.substr(0, path.find_last_of("/\\") + 1);
    NLMISC::CIFile zoneFile;
    if (!zoneFile.open(path))
    {
        nlwarning("Can't open the file for reading: %s", path.c_str());
        return false;
    }
    
    NLMISC::CAABBox bbox;
    NL3D::CZone loadingZone = NL3D::CZone();
    loadingZone.serial(zoneFile);
    zoneFile.close();

    const uint16 zoneId(loadingZone.getZoneId());
    this->landscape->addZone(loadingZone);
    // add neighbor zones to get the same border vertices
    this->addNeighborZones(zoneId, zoneSearchDirectory);
    try
    {
        this->loadTileBank(this->tileBankFilePath);
    }
    catch (...)
    {
        nlerror("Can't load bankfile: %s", this->tileBankFilePath.c_str());
        return false;
    }

    const sint zoneX(zoneId & 255);
    const sint zoneY(zoneId >> 8);
    NLMISC::CVector zoneOffset(160.0f * zoneX, -160.0f * zoneY, 0.0f);

    this->createTileIdMap(*this->tileIdMaps[0], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);
    this->createTileIdMap(*this->tileIdMaps[1], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);
    this->createTileIdMap(*this->tileIdMaps[2], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);

    for (sint patchIndex = 0; patchIndex < loadingZone.getNumPatchs(); patchIndex++)
    {
        this->buildFaces(zoneId, patchIndex);
    }

    return true;
}

bool LandscapeManager::LoadShaders(SDL_Window* window)
{
    Shader* landscapeVert = new Shader();
    landscapeVert->Load(this->device, "General.vert");
    Shader* landscapeFrag = new Shader();
    landscapeFrag->Load(this->device, "General.frag");

    // Set up pipeline
    SDL_GPUTextureFormat colorTargetFormat = SDL_GetGPUSwapchainTextureFormat(this->device, window);
    SDL_GPUTextureFormat depthTargetFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    SDL_GPUSampleCount msaaSampleCount = SDL_GPU_SAMPLECOUNT_1;
    if (!SDL_GPUTextureSupportsSampleCount(this->device, colorTargetFormat, msaaSampleCount))
    {
        SDL_Log("Sample count %d is not supported", (1 << static_cast<int>(msaaSampleCount)));
        msaaSampleCount = SDL_GPU_SAMPLECOUNT_1;
    }

    SDL_GPUVertexBufferDescription landscapeVBD[1] = {
        {
            0,
            sizeof(VertexData),
            SDL_GPU_VERTEXINPUTRATE_VERTEX,
            0
        }
    };

    SDL_GPUVertexAttribute landscapeVA[7] = {
        // Position
        {
            0, // location
            0, // slot
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, // format
            0 // offset
        },
        // Normal
        {
            1,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            sizeof(float) * 3
        },
        // Main UV
        {
            2,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            sizeof(float) * 6
        },
        // Tile Index 0
        {
            3,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_INT3,
            sizeof(float) * 8
        },
        // 4: Tile 0 UV
        {
            4,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            sizeof(float) * 8 + sizeof(uint32) * 3
        },
        // 5: Tile 1 UV
        {
            5,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            sizeof(float) * 10 + sizeof(uint32) * 3
        },
        // 6: Tile 2 UV
        {
            6,
            0,
            SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            sizeof(float) * 12 + sizeof(uint32) * 3
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo landscapePipelineInfo = SDL_GPUGraphicsPipelineCreateInfo();
    landscapePipelineInfo.vertex_shader = landscapeVert->GetShader();
    landscapePipelineInfo.fragment_shader = landscapeFrag->GetShader();

    landscapePipelineInfo.vertex_input_state = SDL_GPUVertexInputState();
    landscapePipelineInfo.vertex_input_state.vertex_buffer_descriptions = landscapeVBD;
    landscapePipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    landscapePipelineInfo.vertex_input_state.vertex_attributes = landscapeVA;
    landscapePipelineInfo.vertex_input_state.num_vertex_attributes = 7;

    landscapePipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    landscapePipelineInfo.rasterizer_state = SDL_GPURasterizerState();
    landscapePipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    //landscapePipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    //landscapePipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
    landscapePipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    landscapePipelineInfo.multisample_state = SDL_GPUMultisampleState();
    landscapePipelineInfo.multisample_state.sample_count = msaaSampleCount;
    
    landscapePipelineInfo.depth_stencil_state = SDL_GPUDepthStencilState();
    landscapePipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    landscapePipelineInfo.depth_stencil_state.enable_depth_test = true;
    landscapePipelineInfo.depth_stencil_state.enable_depth_write = true;
    
    landscapePipelineInfo.target_info = SDL_GPUGraphicsPipelineTargetInfo();
	SDL_GPUColorTargetDescription colorTargets[1];
    colorTargets[0] = SDL_GPUColorTargetDescription();
    colorTargets[0].format = colorTargetFormat;
    landscapePipelineInfo.target_info.color_target_descriptions = colorTargets;
    landscapePipelineInfo.target_info.num_color_targets = 1;
    landscapePipelineInfo.target_info.depth_stencil_format = depthTargetFormat;
    landscapePipelineInfo.target_info.has_depth_stencil_target = true;

    this->landscapePipeline =
        SDL_CreateGPUGraphicsPipeline(this->device, &landscapePipelineInfo);
    if (this->landscapePipeline == NULL)
    {
        SDL_Log("Failed to create textured fill pipeline!");
        return false;
    }

    // Release shaders
    landscapeVert->Release(this->device);
    landscapeFrag->Release(this->device);
    delete landscapeVert;
    delete landscapeFrag;

    return true;
}

bool LandscapeManager::PrepareRender()
{
    SDL_GPUTransferBufferCreateInfo transferInfo;
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = 256 * 256 * 3 * 4;
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(this->device, &transferInfo);

    Uint32 texTransferOffset = 0;
    this->tileIdMaps[0]->Stage(this->device, transferBuffer, texTransferOffset);
    texTransferOffset = this->tileIdMaps[0]->total_byte_count();
    this->tileIdMaps[1]->Stage(this->device, transferBuffer, texTransferOffset);
    texTransferOffset += this->tileIdMaps[1]->total_byte_count();
    this->tileIdMaps[2]->Stage(this->device, transferBuffer, texTransferOffset);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(this->device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    texTransferOffset = 0;
    this->tileIdMaps[0]->Upload(this->device, copyPass, transferBuffer, texTransferOffset);
    texTransferOffset = this->tileIdMaps[0]->total_byte_count();
    this->tileIdMaps[1]->Upload(this->device, copyPass, transferBuffer, texTransferOffset);
    texTransferOffset = this->tileIdMaps[1]->total_byte_count();
    this->tileIdMaps[2]->Upload(this->device, copyPass, transferBuffer, texTransferOffset);

    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = this->tileImages->layers * 4096 * 4096 * 4;
    SDL_GPUTransferBuffer* arrTransfer =
        SDL_CreateGPUTransferBuffer(this->device, &transferInfo);

    for (int i = 0; i < this->tileImages->layers; i++)
    {
        int offset = i * 4096 * 4096 * 4;
        this->tileImages->StageLayer(this->device, arrTransfer, i, offset);
        this->tileImages->Upload(this->device, copyPass, arrTransfer, offset, i);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(this->device, transferBuffer);

    this->createBuffer(this->vertices.data(),
        this->vertices.size() * sizeof(VertexData),
        SDL_GPU_BUFFERUSAGE_VERTEX, this->vertexBuffer);
    this->createBuffer(this->indexes.data(),
        this->indexes.size() * sizeof(int),
        SDL_GPU_BUFFERUSAGE_INDEX, this->indexBuffer);

    SDL_GPUSamplerCreateInfo samplerCreateInfo = SDL_GPUSamplerCreateInfo();
    samplerCreateInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerCreateInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerCreateInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    //samplerCreateInfo.max_anisotropy = 4;
    //samplerCreateInfo.min_lod = 0.0f;
    //samplerCreateInfo.max_lod = 200.0f;
    //samplerCreateInfo.enable_anisotropy = true;
    this->samplers[0] = SDL_CreateGPUSampler(this->device, &samplerCreateInfo);
    this->samplers[1] = SDL_CreateGPUSampler(this->device, &samplerCreateInfo);
    this->samplers[2] = SDL_CreateGPUSampler(this->device, &samplerCreateInfo);
    this->samplers[3] = SDL_CreateGPUSampler(this->device, &samplerCreateInfo);

    this->ready = true;
    
    return true;
}

bool LandscapeManager::Render(SDL_GPURenderPass* renderPass,
    SDL_GPUCommandBuffer* cmd)
{
    if (!this->ready)
    {
        return true;
    }

    SDL_PushGPUVertexUniformData(cmd, 1, &this->worldTransform, sizeof(Mat4));

    if (this->landscapePipeline == nullptr)
    {
        SDL_Log("pipeline is null");
        return false;
    }
    if (this->indexBuffer == nullptr)
    {
        SDL_Log("index buffer is null");
        return false;
    }
    if (this->vertexBuffer == nullptr)
    {
        SDL_Log("vertex buffer is null");
        return false;
    }

    SDL_BindGPUGraphicsPipeline(renderPass, this->landscapePipeline);
    SDL_GPUBufferBinding vertexBufferBindings[1];
    vertexBufferBindings[0] = SDL_GPUBufferBinding();
    vertexBufferBindings[0].buffer = this->vertexBuffer;
    vertexBufferBindings[0].offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBindings, 1); // Vertex buffer size!
    
    SDL_GPUTextureSamplerBinding textureSamplerBindings[4];
    textureSamplerBindings[0] = SDL_GPUTextureSamplerBinding();
    textureSamplerBindings[0].texture = this->tileIdMaps[0]->texture;
    textureSamplerBindings[0].sampler = this->samplers[0];
    textureSamplerBindings[1] = SDL_GPUTextureSamplerBinding();
    textureSamplerBindings[1].texture = this->tileIdMaps[1]->texture;
    textureSamplerBindings[1].sampler = this->samplers[1];
    textureSamplerBindings[2] = SDL_GPUTextureSamplerBinding();
    textureSamplerBindings[2].texture = this->tileIdMaps[2]->texture;
    textureSamplerBindings[2].sampler = this->samplers[2];
    textureSamplerBindings[3].texture = this->tileImages->texture;
    textureSamplerBindings[3].sampler = this->samplers[3];

    SDL_GPUBufferBinding indexBufferBinding = SDL_GPUBufferBinding();
    indexBufferBinding.buffer = this->indexBuffer;
    indexBufferBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding,
        SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 4);
    SDL_DrawGPUIndexedPrimitives(renderPass, this->indexes.size(), 1, 0, 0, 0);

    return true;
}

void LandscapeManager::parsePath(std::string& path)
{
    path = path.substr(path.find(":") + 1);
    path.erase(path.find_last_not_of(" \n\r\t,\"") + 1);
}

void LandscapeManager::addZone(const std::string& zoneSearchDirectory,
    const sint x, const sint y)
{
    std::string zoneFilename(zoneSearchDirectory);
    zoneFilename += this->zoneName(x, y);
    zoneFilename += ".zonel";

    NLMISC::CIFile zoneFile;
    if (zoneFile.open(zoneFilename))
    {
        nlinfo("Found Neighbor Zone: %s", zoneFilename.c_str());
        NL3D::CZone zone;
        zone.serial(zoneFile);
        //this->landscapeModel->Landscape.addZone(zone);
        this->landscape->addZone(zone);
        zoneFile.close();
        return;
    }

    zoneFilename = zoneSearchDirectory;
    zoneFilename += this->zoneNameLowerCase(x, y);
    zoneFilename += ".zonel";
    if (zoneFile.open(zoneFilename))
    {
        nlinfo("Found Neighbor Zone: %s", zoneFilename.c_str());
        NL3D::CZone zone;
        zone.serial(zoneFile);
        //this->landscapeModel->Landscape.addZone(zone);
        this->landscape->addZone(zone);
        zoneFile.close();
    }
}

std::string LandscapeManager::zoneName(const sint x, const sint y)
{
	std::ostringstream name;

	name << y + 1 << "_" << static_cast<char>('A' + (x / 26)) << static_cast<char>('A' + (x % 26));

	return name.str();
}

std::string LandscapeManager::zoneNameLowerCase(const sint x, const sint y)
{
    std::ostringstream name;

    name << y + 1 << "_" << static_cast<char>('a' + (x / 26)) << static_cast<char>('a' + (x % 26));

    return name.str();
}

void LandscapeManager::addNeighborZones(const uint16& zoneId,
    const std::string& zoneSearchDirectory)
{
    const sint x(zoneId & 255);
    const sint y(zoneId >> 8);

    this->addZone(zoneSearchDirectory, x - 1, y - 1);
    this->addZone(zoneSearchDirectory, x + 0, y - 1);
    this->addZone(zoneSearchDirectory, x + 1, y - 1);
    this->addZone(zoneSearchDirectory, x - 1, y + 0);
    this->addZone(zoneSearchDirectory, x + 0, y + 0);
    this->addZone(zoneSearchDirectory, x + 1, y + 0);
    this->addZone(zoneSearchDirectory, x - 1, y + 1);
    this->addZone(zoneSearchDirectory, x + 0, y + 1);
    this->addZone(zoneSearchDirectory, x + 1, y + 1);
}

void LandscapeManager::loadTileBank(const std::string& bankFilePath)
{
    if (!bankFilePath.empty())
    {
        NLMISC::CIFile bankFile(bankFilePath);
        this->tileBank->serial(bankFile);
        nldebug("TileBank land count %i", this->tileBank->getLandCount());
        nldebug("TileBank tileSet count %i", this->tileBank->getTileSetCount());
        nldebug("TileBank tile count %i", this->tileBank->getTileCount());

        // Store unique filename
        std::unordered_set<std::string> tileFiles;
        // Tileid to filename set pos
        std::vector<std::string> tileplaces;

        sint tilecount = this->tileBank->getTileCount();
        // First get our filenames, make sure we only have unique filenames
        // else we will keep open same files.
        for (sint i = 0; i < tilecount; i++)
        {
            std::string tfs = this->tileBank->getTile(i)->getRelativeFileName(NL3D::CTile::diffuse);
            auto k = tileFiles.insert(tfs);
            // Map tileid to the place string is stored
            tileplaces.push_back(tfs);
        }

        // Filename to either bm128 (false) or bm256 (true)
        std::map<std::string, std::pair<bool, int>> fnlookup;
        // Two arrays so we can sort
        std::vector<NL3D::CBitmap> bm128;
        std::vector<NL3D::CBitmap> bm256;
        // Dummy for anything not found
        bm128.push_back(NL3D::CBitmap());
        bm128[0].makeDummy();
        bm128[0].resample(128, 128);

        // Open file, get the bitmaps out
        // Sort bitmaps whatever they are 128 or 256
        for (std::unordered_set<std::string>::iterator tfiter = tileFiles.begin();
            tfiter != tileFiles.end();
            tfiter++)
        {
            NL3D::CBitmap tbbm;
            std::string fname = this->tileBank->getAbsPath() + *tfiter;
            NL3D::CTextureFile::buildBitmapFromFile(tbbm, fname, false);

            uint32 h = tbbm.getHeight();

            if (h == 256)
            {
                bm256.push_back(tbbm);
                // We will use the lookup for the tileplaces later
                fnlookup.insert({ *tfiter, { true, static_cast<int>(bm256.size() - 1) } });
            }
            else if (h == 128)
            {
                bm128.push_back(tbbm);
                // Same, for tileplaces
                fnlookup.emplace(*tfiter,
                    std::pair<bool, int>(false, static_cast<int>(bm128.size() - 1)));
            }
            else
            {
                fnlookup.emplace(*tfiter, std::pair<bool, int>(false, 0));
            }
        }

        // Now, the tileplaces earlir, we do not need the strings anymore
        // just tell where we can find the tile
        std::vector<std::pair<bool, int>> tileto2arrays;

        for (auto tfr : tileplaces)
        {
            tileto2arrays.push_back(fnlookup[tfr]);
        }

        // Cleanup
        tileplaces.clear();
        tileFiles.clear();

        // Now the next task, try to determine how many layers we need

        // Number of 128s we can put in a 4096 is 31 * 31 = 961
        // 31 = 4096 / 128 + 1
        // TODO: If we does not have any 128 or 256, lenght should be 0
        int times128 = std::max(static_cast<int>(ceil(bm128.size() / 961)), 1);
        // 15 = 4096 / 256 + 1, 15 * 15 = 225
        int times256 = std::max(static_cast<int>(ceil(bm256.size() / 225)), 1);
        
        this->tileImages = new Image();
        this->tileImages->type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        this->tileImages->width = 4096;
        this->tileImages->height = this->tileImages->width;
        this->tileImages->component = 4;
        this->tileImages->layers = (times128 + times256);
        this->tileImages->Prepare(this->device);
        this->tileImages->pixels.resize(
            this->tileImages->width * this->tileImages->height * this->tileImages->layers);

        std::vector<std::array<float, 3>> uv128lookup;
        std::vector<std::array<float, 3>> uv256lookup;

        int h = 128;
        int layer = 0;
        int offsetx = 0;
        int offsety = 0;
        float uvstep = 31.0f / 4096.0f;
        for (NL3D::CBitmap bm : bm128)
        {
            NLMISC::CObjectVector<uint8>& pixels = bm.getPixels();

            int baseAtlasX = offsetx * 130 + 1;
            int baseAtlasY = offsety * 130 + 1;
            int layerOffset = layer * 4096 * 4096;

            uv128lookup.push_back({ baseAtlasX / 4096.0f, baseAtlasY / 4096.0f,
                static_cast<float>(layer) });

            for (int y = 0; y < h; y++)
            {
                int atlasRowIndex = (baseAtlasY + y) * 4096;
                for (int x = 0; x < h; x++)
                {
                    int srcIdx = (y * h + x) * 4;
                    int destIdx = layerOffset + atlasRowIndex + (baseAtlasX + x);
                    
                    this->tileImages->pixels[destIdx] =
                        (pixels[srcIdx + 3] << 24) |
                        (pixels[srcIdx + 2] << 16) |
                        (pixels[srcIdx + 1] << 8) |
                        pixels[srcIdx];
                }
            }

            offsetx++;
            
            if (offsetx > 31)
            {
                offsetx = 0;
                offsety++;

                if (offsety > 31)
                {
                    offsety = 0;
                    layer++;
                }
            }
        }

        bm128.clear();

        layer++;
        h = 256;
        offsetx = 0;
        offsety = 0;

        for (NL3D::CBitmap bm : bm256)
        {
            NLMISC::CObjectVector<uint8>& pixels = bm.getPixels();

            int baseAtlasX = offsetx * 258 + 1;
            int baseAtlasY = offsety * 258 + 1;
            int layerOffset = layer * 4096 * 4096;

            uv256lookup.push_back({ baseAtlasX / 4096.0f, baseAtlasY / 4096.0f,
                static_cast<float>(layer) });

            for (int x = 0; x < h; x++)
            {
                for (int y = 0; y < h; y++)
                {
                    int srcIdx = (y * h + x) * 4;
                    int destIdx = layerOffset + ((baseAtlasY + y) * 4096) + (baseAtlasX + x);
                    
                    this->tileImages->pixels[destIdx] =
                        (pixels[srcIdx + 3] << 24) |
                        (pixels[srcIdx + 2] << 16) |
                        (pixels[srcIdx + 1] << 8) |
                        pixels[srcIdx];
                }
            }

            offsetx++;
            if (offsetx > 15)
            {
                offsetx = 0;
                offsety++;

                if (offsety > 15)
                {
                    offsety = 0;
                    layer++;
                }
            }
        }

        bm256.clear();

        // Finally, we can put locations so that we can find them later
        for (auto tta : tileto2arrays)
        {
            if (tta.first)
            {
                this->tileBitmaps.push_back(uv256lookup[tta.second]);
            }
            else
            {
                this->tileBitmaps.push_back(uv128lookup[tta.second]);
            } 
        }
    }
}

uint8 LandscapeManager::getPatchTileIndex(const NL3D::CPatch& patch, const uint8 s, const uint8 t)
{
    return t * patch.getOrderS() + s;
}

// check CTessFace::initTileUvRGBA for correct calculation
NLMISC::CUV LandscapeManager::tileOrientation(NLMISC::CUV in, uint8 orientation)
{
    switch (orientation)
    {
    default:
    case 0:
        return { in.U, in.V };
    case 1:
        return { 1 - in.V, in.U };
    case 2:
        return { 1 - in.U, 1 - in.V };
    case 3:
        return { in.V, 1 - in.U };
    }
}

NLMISC::CUV LandscapeManager::tileUV(const NLMISC::CUV &in, uint8 orientation,
    bool is256, uint8 uvOff)
{
    NLMISC::CUV out(in);
    if (is256)
    {
        out *= 0.5;
        // with rotation applied afterward we need to reverse the already applied rotation in uvOff
        uvOff = (uvOff + orientation) & 3;
        if (uvOff == 2 || uvOff == 3)
        {
            out.U += 0.5;
        }
        if (uvOff == 1 || uvOff == 2)
        {
            out.V += 0.5;
        }
    }
    // Do the HalfPixel scale bias.
    float hBiasXY, hBiasZ;
    if (is256)
    {
        hBiasXY = NL3D::CLandscapeGlobals::TilePixelBias256;
        hBiasZ = NL3D::CLandscapeGlobals::TilePixelScale256;
    }
    else
    {
        hBiasXY = NL3D::CLandscapeGlobals::TilePixelBias128;
        hBiasZ = NL3D::CLandscapeGlobals::TilePixelScale128;
    }

    hBiasXY = 0.0f;
    hBiasZ = 1.0f;

    // Scale the UV.
    out.U *= hBiasZ;
    out.V *= hBiasZ;
    out.U += hBiasXY;
    out.V += hBiasXY;

    return out;
}

void LandscapeManager::drawImage(Image& target, int x, int y, Image& part)
{
    for (int iy = y; iy < target.height && iy < part.height; iy++)
    {
        for (int ix = x; ix < target.width && ix < part.width; ix++)
        {
            target.pixels[ix * iy * target.width] =
                part.pixels[ix - x + (iy - y) * part.width];
        }
    }
}

void LandscapeManager::createTileIdMap(Image& image, int width, int height)
{
    image.width = width;
    image.height = height;
    image.component = 8;
    image.Prepare(this->device);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            image.pixels[y * width + x] =
                (static_cast<Uint64>(NL_TILE_ELM_LAYER_EMPTY) << 48) |
                (static_cast<Uint64>(0) << 32) |
                (static_cast<Uint64>(0) << 16) |
                static_cast<Uint64>(NL_TILE_ELM_LAYER_EMPTY);
        }
    }
}

void LandscapeManager::drawTileInfoMap(
    const NL3D::CPatch& patch, Image& image, uint8 layer)
{
    const auto &tiles = patch.Tiles;
    int w = patch.getOrderT();

    for (auto y = 0; y < w; y++)
    {
        for (auto x = 0; x < patch.getOrderS(); x++)
        {
            auto tileIndex = this->getPatchTileIndex(patch, x, y);
            const auto& tile = tiles[tileIndex];
            const auto orientation = tile.getTileOrient(layer);
            const auto tileId = tile.Tile[layer];
            uint8 rotAlpha = 0;
            if (tileId != NL_TILE_ELM_LAYER_EMPTY)
            {
                rotAlpha = this->tileBank->getTile(tileId)->getRotAlpha();
            }
            image.pixels[y * w + x] =
                (static_cast<Uint64>(tileId) << 48) |
                (static_cast<Uint64>(orientation) << 32) |
                (static_cast<Uint64>(rotAlpha) << 16) |
                static_cast<Uint64>(NL_TILE_ELM_LAYER_EMPTY);
        }
    }
}

void LandscapeManager::buildFaces(sint zoneId, sint patch)
{
    NLMISC::CUV A(0, 0), B(0, 1), C(1, 1), D(1, 0);
    NL3D::CZone *pZone = this->landscape->getZone(zoneId); //this->landscapeModel->Landscape.getZone(zoneId);

    // Then trace all patch.
    nlassert(patch >= 0);
    nlassert(patch < pZone->getNumPatchs());
    const NL3D::CPatch* pa = const_cast<const NL3D::CZone*>(pZone)->getPatch(patch);
    const auto& tiles = pa->Tiles;
    NL3D::CBezierPatch bezierPatch;
    pa->unpack(bezierPatch);

    // Build the faces.
    //=================
    uint8 ordS = pa->getOrderS();
    uint8 ordT = pa->getOrderT();
    uint16 patchOffset(patch * this->PATCH_SIZE);
    uint8 x, y;
    float pixelOffset = 0.125f / this->TILE_ID_MAP_SIZE;
    float OOS = 1.0f / ordS;
    float OOT = 1.0f / ordT;

    uint16 offset_x(patchOffset % TILE_ID_MAP_SIZE);
    uint16 offset_y((patchOffset / TILE_ID_MAP_SIZE) * PATCH_SIZE);
    Image tileInfoMapPatch;
    this->createTileIdMap(tileInfoMapPatch, PATCH_SIZE, PATCH_SIZE);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 0);
    this->drawImage(*this->tileIdMaps[0], offset_x, offset_y, tileInfoMapPatch);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 1);
    this->drawImage(*this->tileIdMaps[1], offset_x, offset_y, tileInfoMapPatch);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 2);
    this->drawImage(*this->tileIdMaps[2], offset_x, offset_y, tileInfoMapPatch);

    for (y = 0; y < ordT; y++)
    {
        for (x = 0; x < ordS; x++)
        {
            uint8 tileIndex = this->getPatchTileIndex(*pa, x, y);
            const auto& tile = tiles[tileIndex];
            if (tile.Tile[0] == NL_TILE_ELM_LAYER_EMPTY)
            {
                nlwarning("tile base layer not defined patch %d x %d y %d tileIndex %d",
                    patch, x, y, tileIndex);
            }
            float imageX = offset_x + x;
            float imageY = offset_y + y;
            bool is256;
            uint8 uvOff;
            tile.getTile256Info(is256, uvOff);

            auto& element = this->tileBitmaps.at(tile.Tile[0]);
            float tileTextureInfo[3] = {
                element.at(0),
                element.at(1),
                element.at(2)
            };
            float uvScale = 0;
            if (is256)
            {
                uvScale = 256.0f / 4096.0f;
            }
            else
            {
                uvScale = 128.0f / 4096.0f;
            }
        
            uint32_t vertexOffset = this->vertices.size();
            this->indexes.push_back(vertexOffset + 0);
            this->indexes.push_back(vertexOffset + 1);
            this->indexes.push_back(vertexOffset + 2);
            this->indexes.push_back(vertexOffset + 0);
            this->indexes.push_back(vertexOffset + 2);
            this->indexes.push_back(vertexOffset + 3);

            VertexData vd = VertexData();
            vd.position = NL3D::CVector(pa->computeContinousVertex(x * OOS, y * OOT));
            vd.normal = NL3D::CVector(bezierPatch.evalNormal(x * OOS, y * OOT));
            vd.mainUV[0] = tileTextureInfo[0] + imageX * uvScale;
            vd.mainUV[1] = tileTextureInfo[1] + imageY * uvScale;
            vd.mainUV[2] = tileTextureInfo[2];
            vd.tileIndexes[0] = tile.Tile[0];
            vd.tileIndexes[1] = tile.Tile[1];
            vd.tileIndexes[2] = tile.Tile[2];
            vd.tileUV0 = this->tileUV(A, tile.getTileOrient(0), is256, uvOff);
            vd.tileUV1 = this->tileUV(A, tile.getTileOrient(1), is256, uvOff);
            vd.tileUV2 = this->tileUV(A, tile.getTileOrient(2), is256, uvOff);
            this->vertices.push_back(vd);

            vd.position = NL3D::CVector(pa->computeContinousVertex(x * OOS, (y + 1) * OOT));
            vd.normal = NL3D::CVector(bezierPatch.evalNormal(x * OOS, (y + 1) * OOT));
            vd.mainUV[1] = tileTextureInfo[1] + (imageY + pixelOffset) * uvScale;
            vd.tileUV0 = this->tileUV(B, tile.getTileOrient(0), is256, uvOff);
            vd.tileUV1 = this->tileUV(B, tile.getTileOrient(1), is256, uvOff);
            vd.tileUV2 = this->tileUV(B, tile.getTileOrient(2), is256, uvOff);
            this->vertices.push_back(vd);

            vd.position = NL3D::CVector(pa->computeContinousVertex((x + 1) * OOS, (y + 1) * OOT));
            vd.normal = NL3D::CVector(bezierPatch.evalNormal((x + 1) * OOS, (y + 1) * OOT));
            vd.mainUV[0] = tileTextureInfo[0] + (imageX + pixelOffset) * uvScale;
            vd.tileUV0 = this->tileUV(C, tile.getTileOrient(0), is256, uvOff);
            vd.tileUV1 = this->tileUV(C, tile.getTileOrient(1), is256, uvOff);
            vd.tileUV2 = this->tileUV(C, tile.getTileOrient(2), is256, uvOff);
            this->vertices.push_back(vd);

            vd.position = NL3D::CVector(pa->computeContinousVertex((x + 1) * OOS, y * OOT));
            vd.normal = NL3D::CVector(bezierPatch.evalNormal((x + 1) * OOS, y * OOT));
            vd.mainUV[1] = tileTextureInfo[1] + imageY * uvScale;
            vd.tileUV0 = this->tileUV(D, tile.getTileOrient(0), is256, uvOff);
            vd.tileUV1 = this->tileUV(D, tile.getTileOrient(1), is256, uvOff);
            vd.tileUV2 = this->tileUV(D, tile.getTileOrient(2), is256, uvOff);
            this->vertices.push_back(vd);
        }
    }
}

void LandscapeManager::createBuffer(
    const void* data, size_t size,
    SDL_GPUBufferUsageFlags usage, SDL_GPUBuffer*& buffer)
{
    SDL_GPUBufferCreateInfo bufferCreateInfo = SDL_GPUBufferCreateInfo();
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.size = static_cast<Uint32>(size);
    buffer = SDL_CreateGPUBuffer(this->device, &bufferCreateInfo);
    
    SDL_GPUTransferBufferCreateInfo transferInfo = SDL_GPUTransferBufferCreateInfo();
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(size);
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(this->device, &transferInfo);

    void* transferData = SDL_MapGPUTransferBuffer(this->device, transferBuffer, false);
    memcpy(transferData, data, size);
    SDL_UnmapGPUTransferBuffer(this->device, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(this->device);
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
}
