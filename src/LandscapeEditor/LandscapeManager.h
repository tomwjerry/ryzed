#pragma once
#include "nel/ligo/ligo_config.h"
#include "nel/ligo/primitive.h"
#include "nel/ligo/zone_region.h"
#include <string>

class LandscapeManager
{
private:
    std::string editLandscapePath;
    std::string editZoneRegionPath;
    std::string editLandscapePrimitivePath;

    NLLIGO::CPrimitives* editLandscapePrimitive;
    NLLIGO::CZoneRegion editZoneRegion;
    NLLIGO::CLigoConfig* ligoConfig;

    void parsePath(std::string& path);

public:
    LandscapeManager(const std::string& cfgFile);
    ~LandscapeManager();

    void CreateLandscape();
 
    void Load(const std::string& path);
    void LoadLandscapePrimitive(const std::string& path);
    void LoadZoneRegion(const std::string& path);

    void ConvertLandscapePrimitive(const std::string& path);

    void Save();
    void Save(const std::string& path);
    void SaveLandscapePrimitive();
    void SaveLandscapePrimitive(const std::string& path);
    void SaveZoneRegion();
    void SaveZoneRegion(const std::string& path);
};
