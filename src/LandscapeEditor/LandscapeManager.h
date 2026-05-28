#pragma once
#include "nel/3d/zone.h"
#include <string>
#include <vector>

struct TileData
{
    uint16 tileId;
    CUV uv;
};

struct VertexData
{
    CVector position;
    CVector normal;
    CUV tileInfoUv;
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

    std::string editLandscapePath;
    std::string editZoneRegionPath;
    std::string editLandscapePrimitivePath;

    std::vector<NL3D::CZone*> editZone;
    /*NLLIGO::CPrimitives* editLandscapePrimitive;
    NLLIGO::CZoneRegion editZoneRegion;
    NLLIGO::CLigoConfig* ligoConfig;*/

    void parsePath(std::string& path);

public:
    LandscapeManager(const std::string& cfgFile);
    ~LandscapeManager();

    void Load(const std::string& path);
    void LoadZoneDir(const std::string& path);
    void LoadZone(const std::string& path);

    /*void CreateLandscape();
 
    void Load(const std::string& path);
    void LoadLandscapePrimitive(const std::string& path);
    void LoadZoneRegion(const std::string& path);

    void ConvertLandscapePrimitive(const std::string& path);

    void Save();
    void Save(const std::string& path);
    void SaveLandscapePrimitive();
    void SaveLandscapePrimitive(const std::string& path);
    void SaveZoneRegion();
    void SaveZoneRegion(const std::string& path);*/
};
