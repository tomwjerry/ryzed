#pragma once

#include "../Math.h"
#include "../3D/Image.h"
#include "../3D/Material.h"
#include "../3D/IRenderable.h"

#include "nel/3d/landscape_model.h"
#include "nel/3d/landscape.h"
#include "nel/3d/zone.h"

#include <SDL3/SDL.h>

#include <array>
#include <map>
#include <vector>

/** 
 * AGPL(?), Based on the code by zerotacg 
 */

struct VertexData
{
    NLMISC::CVector position;
    NLMISC::CVector normal;
    float mainUV[3];
    uint32 tileIndexes[3];
    NLMISC::CUV tileUV0;
    NLMISC::CUV tileUV1;
    NLMISC::CUV tileUV2;
};

class LandscapeManager : public IRenderable
{
private:
    const uint8 TILE_LAYER_COUNT = 3;
    const uint16 TILE_ID_MAP_SIZE = 256;
    const uint16 PATCH_SIZE = 16;
    const uint16 NORMAL_SIZE = PATCH_SIZE * 4;
    const uint16 NORMAL_MAP_SIZE = TILE_ID_MAP_SIZE * 4;

    SDL_GPUDevice* device;

    Mat4 worldTransform;

    std::string tileBankFilePath;

    NL3D::CTileBank* tileBank;
    NL3D::CLandscape* landscape;

    bool ready = false;

    std::vector<VertexData> vertices;
    std::vector<int> indexes;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    
    SDL_GPUGraphicsPipeline* landscapePipeline;

    std::vector<std::array<float, 3>> tileBitmaps;
    Image* tileImages;
    Image* tileIdMaps[3];
    SDL_GPUSampler* samplers[4];

    void parsePath(std::string& path);

    void addZone(const std::string& zoneSearchDirectory,
        const sint x, const sint y);
    std::string zoneName(const sint x, const sint y);
    std::string zoneNameLowerCase(const sint x, const sint y);
    void addNeighborZones(const uint16& zoneId,
        const std::string& zoneSearchDirectory);
    void loadTileBank(const std::string& bankFilePath);

    uint8 getPatchTileIndex(const NL3D::CPatch& patch, const uint8 s, const uint8 t);
    NLMISC::CUV tileOrientation(NLMISC::CUV in, uint8 orientation);
    NLMISC::CUV tileUV(const NLMISC::CUV &in, uint8 orientation, bool is256, uint8 uvOff);

    void drawImage(Image& target, int x, int y, Image& part);
    void createTileIdMap(Image& image, int width, int height);
    void drawTileInfoMap(const NL3D::CPatch& patch, Image& image, uint8 layer);
    
    void buildFaces(sint zoneId, sint patchIndex);

    void createBuffer(
        const void* data, size_t size,
        SDL_GPUBufferUsageFlags usage, SDL_GPUBuffer*& buffer);

public:
    LandscapeManager(SDL_GPUDevice* device, const std::string& cfgFile);
    ~LandscapeManager();

    bool Load(const std::string& path);
    
    bool LoadZoneDir(const std::string& path);
    bool LoadZone(const std::string& path);

    virtual bool LoadShaders(SDL_Window* window);
    virtual bool PrepareRender();
    virtual bool Render(SDL_GPURenderPass* renderPass,
        SDL_GPUCommandBuffer* cmd);
};
