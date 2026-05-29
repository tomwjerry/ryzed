#pragma once

#include "Image.h"

#include "nel/3d/zone.h"

#include <string>
#include <vector>

/** 
 * AGPL(?), Based on the code by zerotacg 
 */

struct TileData
{
    uint16 tileId;
    NLMISC::CUV uv;
};

struct VertexData
{
    NLMISC::CVector position;
    NLMISC::CVector normal;
    NLMISC::CUV tileInfoUv;
    TileData tile[TILE_LAYER_COUNT];
};

class LandscapeManager
{
private:
    const uint8 TILE_LAYER_COUNT = 3;
    const uint16 TILE_ID_MAP_SIZE = 256;
    const uint16 PATCH_SIZE = 16;
    const uint16 NORMAL_SIZE = PATCH_SIZE * 4;
    const uint16 NORMAL_MAP_SIZE = TILE_ID_MAP_SIZE * 4;

    std::vector<NL3D::CZone*> editZone;

    SDL_GPUBuffer* vbos[2];

    void parsePath(std::string& path);

    uint8 getPatchTileIndex(const NL3D::CPatch& patch, const uint8 s, const uint8 t);
    NLMISC::CUV tileOrientation(NLMISC::CUV in, uint8 orientation);
    NLMISC::CUV tileUV(const NLMISC::CUV &in, uint8 orientation, bool is256, uint8 uvOff);
    void setPixel(Image& image, int x, int y, uint16 grayscale);
    
    void createNormalMap(Image* image, int width, int height);
    void drawNormalMap(const NL3D::CPatch& patch, Image& image);
    
    void drawImage(Image& target, int x, int y, Image& part);

    void createTileIdMap(Image* image, int width, int height);
    uint8 getTileOrientation(
        const NL3D::CPatch& patch, const NL3D::CTileElement& tile, const uint8 layer);
    void drawTileInfoMap(const NL3D::CPatch& patch, Image& image, uint8 layer);
    
    void buildFaces(
        NL3D::CLandscape &landscape, sint zoneId, sint patch,
        OutputData &output, Image* tileIdMap, Image& normalMap);
    void addZone(NL3D::CLandscape& landscape, const std::string& zoneSearchDirectory,
        const sint x, const sint y);
    void addNeighborZones(NL3D::CLandscape& landscape, const uint16& zoneId,
        const std::string& zoneSearchDirectory);
    void loadTileBank(NL3D::CLandscape& landscape, const std::string& bankFilePath);

    SDL_GPUBuffer* createBuffer(
        SDL_GPUDevice* device, const void* data, size_t size,
        SDL_GPUBufferUsageFlags usage);

public:
    LandscapeManager(const std::string& cfgFile);
    ~LandscapeManager();

    bool Load(const std::string& path);
    
    bool LoadZoneDir(const std::string& path);
    bool LoadZone(const std::string& path);
};
