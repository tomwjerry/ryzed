/** 
 * AGPL(?), Based on the code by zerotacg
 */

#include "LandscapeManager.h"

#include <istream>
#include <fstream>
#include <filesystem>

#include "nel/ligo/primitive_utils.h"
#include "nel/misc/path.h"
#include "nel/misc/config_file.h"
#include "nel/misc/file.h"
#include "nel/misc/i_xml.h"
#include "nel/misc/types_nl.h"
#include "nel/misc/o_xml.h"
#include "nel/misc/common.h"
#include "nel/misc/cmd_args.h"
#include "nel/misc/bitmap.h"
#include "nel/3d/bezier_patch.h"
#include "nel/3d/landscape.h"
#include "nel/ligo/zone_region.h"

LandscapeManager::LandscapeManager(const std::string& cfgFile)
{
    NLMISC::CPath::addSearchPath("D:/ryzed/build/Debug");
    NLMISC::CPath::addSearchPath("D:/ryzomcore-quickstart-4.0-pre3/leveldesign/primitives");
    /*this->ligoConfig = new NLLIGO::CLigoConfig();
    NLLIGO::CPrimitiveContext::instance().CurrentLigoConfig = this->ligoConfig;
    this->editLandscapePrimitive = new NLLIGO::CPrimitives();
    this->ligoConfig->readPrimitiveClass(cfgFile, false);
    NLLIGO::CPrimitiveContext::instance().CurrentPrimitive = this->editLandscapePrimitive;*/
}

LandscapeManager::~LandscapeManager()
{
    //delete this->ligoConfig;
    //delete this->editLandscapePrimitive;
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

bool LandscapeManager::LoadZoneDir(const std::string& path);
{
    for (const auto& entry : fs::directory_iterator(path))
    {
        this->LoadZone(entry.path().string());
    }

    return true;
}

bool LandscapeManager::LoadZone(const std::string& path)
{
    NLMISC::CIFile zoneFile;
    if (!zoneFile.open(path))
    {
        nlwarning("Can't open the file for reading: %s", path.c_str());
        return false;
    }
    
    NL3D::CLandscape landscape;
    NLMISC::CAABBox bbox;
    NL3D::CZone loadingZone;
    loadingZone.serial(zoneFile);
    zoneFile.close();

    const auto zoneId(loadingZone.getZoneId());
    landscape.setNoiseMode(false);
    // add neighbor zones to get the same border vertices
    this->addNeighborZones(landscape, zoneId, zoneSearchDirectory);
    auto zone = landscape.getZone(zoneId);
    if (zone == nullptr)
    {
        nlerror("Can't find zone with id: %i", zoneId);
        return false;
    }
    try
    {
        this->loadTileBank(landscape, bankFilePath);
    }
    catch (const Exception &)
    {
        nlerror("Can't load bankfile: %s", bankFilePath.c_str());
        return false;
    }

    const sint zoneX(zoneId & 255);
    const sint zoneY(zoneId >> 8);
    NLMISC::CVector zoneOffset(160.0f * zoneX, -160.0f * zoneY, 0.0f);

    Image* tileIdMaps[TILE_LAYER_COUNT];
    this->createTileIdMap(tileIdMaps[0], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);
    this->createTileIdMap(tileIdMaps[1], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);
    this->createTileIdMap(tileIdMaps[2], TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE);

    for (sint patchIndex = 0; patchIndex < zone->getNumPatchs(); patchIndex++)
    {
        this->buildFaces(landscape, zoneId, patchIndex, output, tileIdMaps, normalMap);
    }

    return true;
}

bool LandscapeManager::PrepareRender(SDL_GPUDevice* device)
{
    this->createBuffer(device, this->vertices.data(),
        this->vertices.size() * sizeof(VertexData),
        SDL_GPU_BUFFERUSAGE_VERTEX, this->vbos[0]);
    this->createBuffer(device, this->indexes.data(),
        this->indexes.size() * sizeof(int),
        SDL_GPU_BUFFERUSAGE_INDEX, this->vbos[1]);
}

void LandscapeManager::parsePath(std::string& path)
{
    path = path.substr(path.find(":") + 1);
    path.erase(path.find_last_not_of(" \n\r\t\,\"") + 1);
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

void LandscapeManager::setPixel(Image& image, int x, int y, uint16 grayscale)
{
    nlassert(image.component == 1);
    (uint16)image.pixels[x * y] = grayscale;
}

void LandscapeManager::drawImage(Image& target, int x, int y, Image& part)
{
    for (int iy = y; iy < target.height && iy < part.height; iy++)
    {
        for (int ix = x; ix < target.width && ix < part.width; ix++)
        {
            target.pixels[ix * iy] = part.pixels[ix - x + (iy - y) * part.width];
        }
    }
}

void LandscapeManager::createTileIdMap(Image& image, int width, int height)
{
    image.width = width;
    image.height = height;
    image.component = 8;
    image.Prepare();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            image.pixels[x * y] =
                (static_cast<int>(NL3D::NL_TILE_ELM_LAYER_EMPTY) << 48) |
                (static_cast<int>(0) << 32) |
                (static_cast<int>(0) << 16) |
                static_cast<int>(NL3D::NL_TILE_ELM_LAYER_EMPTY);
        }
    }
}

void LandscapeManager::drawTileInfoMap(
    const NL3D::CPatch& patch, Image& image, uint8 layer)
{
    const auto &tiles = patch.Tiles;
    auto &tileBank = patch.getLandscape()->TileBank;

    for (auto y = 0; y < patch.getOrderT(); y++)
    {
        for (auto x = 0; x < patch.getOrderS(); x++)
        {
            auto tileIndex = this->getPatchTileIndex(patch, x, y);
            const auto& tile = tiles[tileIndex];
            const auto orientation = tile.getTileOrient(layer);
            const auto tileId = tile.Tile[layer];
            uint8 rotAlpha = 0;
            if (tileId != NL3D::NL_TILE_ELM_LAYER_EMPTY)
            {
                rotAlpha = tileBank.getTile(tileId)->getRotAlpha();
            }
            image.pixels[x * y] =
                (static_cast<int>(tileId) << 48) |
                (static_cast<int>(orientation) << 32) |
                (static_cast<int>(rotAlpha) << 16) |
                static_cast<int>(NL3D::NL_TILE_ELM_LAYER_EMPTY);
        }
    }
}

void LandscapeManager::buildFaces(
    NL3D::CLandscape& landscape, sint zoneId, sint patch,
    OutputData& output, Image* tileIdMap, Image& normalMap)
{
    NLMISC::CUV A(0, 0), B(0, 1), C(1, 1), D(1, 0);
    NL3D::CZone *pZone = landscape.getZone(zoneId);

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

    uint16 offset_x(patchOffset % TILE_ID_MAP_SIZE),
    uint16 offset_y((patchOffset / TILE_ID_MAP_SIZE) * PATCH_SIZE);
    Image tileInfoMapPatch;
    this->createTileIdMap(tileInfoMapPatch, PATCH_SIZE, PATCH_SIZE);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 0);
    this->drawImage(tileIdMap[0], offset_x, offset_y, tileInfoMapPatch);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 1);
    this->drawImage(tileIdMap[1], offset_x, offset_y, tileInfoMapPatch);
    this->drawTileInfoMap(*pa, tileInfoMapPatch, 2);
    this->drawImage(tileIdMap[2], offset_x, offset_y, tileInfoMapPatch);


    for (y = 0; y < ordT; y++)
    {
        for (x = 0; x < ordS; x++)
        {
            auto tileIndex = this->getPatchTileIndex(*pa, x, y);
            const auto& tile = tiles[tileIndex];
            if (tile.Tile[0] == NL3D::NL_TILE_ELM_LAYER_EMPTY)
            {
                nlwarning("tile base layer not defined patch %d x %d y %d tileIndex %d",
                    patch, x, y, tileIndex);
            }
            uint16 imageX = offset_x + x;
            uint16 imageY = offset_y + y;
            NLMISC::CUV tileInfo(imageX, imageY);
            NLMISC::CUV b(tileInfo.U, tileInfo.V + pixelOffset);
            NLMISC::CUV c(tileInfo.U + pixelOffset, tileInfo.V + pixelOffset);
            NLMISC::CUV d(tileInfo.U + pixelOffset, tileInfo.V);
            NL3D::CVector uvScaleBias;
            bool is256;
            uint8 uvOff;
            tile.getTile256Info(is256, uvOff);

            NL3D::CVector va(pa->computeContinousVertex(x * OOS, y * OOT));
            NL3D::CVector vb(pa->computeContinousVertex(x * OOS, (y + 1) * OOT));
            NL3D::CVector vc(pa->computeContinousVertex((x + 1) * OOS, (y + 1) * OOT));
            NL3D::CVector vd(pa->computeContinousVertex((x + 1) * OOS, y * OOT));
            NL3D::CVector na(bezierPatch.evalNormal(x * OOS, y * OOT));
            NL3D::CVector nb(bezierPatch.evalNormal(x * OOS, (y + 1) * OOT));
            NL3D::CVector nc(bezierPatch.evalNormal((x + 1) * OOS, (y + 1) * OOT));
            NL3D::CVector nd(bezierPatch.evalNormal((x + 1) * OOS, y * OOT));
        
            this->vertices.push_back({
                va,
                na,
                a,
                {
                    {
                        tile.Tile[0],
                        this->tileUV(A, tile.getTileOrient(0), is256, uvOff)
                    },
                    {
                        tile.Tile[1],
                        this->tileUV(A, tile.getTileOrient(1), is256, uvOff)
                    },
                    {
                        tile.Tile[2],
                        this->tileUV(A, tile.getTileOrient(2), is256, uvOff)
                    }
                }
            });
            this->vertices.push_back({
                vb,
                nb,
                b,
                {
                    {
                        tile.Tile[0],
                        this->tileUV(B, tile.getTileOrient(0), is256, uvOff)
                    },
                    {
                        tile.Tile[1],
                        this->tileUV(B, tile.getTileOrient(1), is256, uvOff)
                    },
                    {
                        tile.Tile[2],
                        this->tileUV(B, tile.getTileOrient(2), is256, uvOff)
                    }
                }
            });
            this->vertices.push_back({
                vc,
                nc,
                c,
                {
                    {
                        tile.Tile[0],
                        this->tileUV(C, tile.getTileOrient(0), is256, uvOff) },
                    {
                        tile.Tile[1],
                        this->tileUV(C, tile.getTileOrient(1), is256, uvOff) },
                    {
                        tile.Tile[2],
                        this->tileUV(C, tile.getTileOrient(2), is256, uvOff)
                    }
                }
            });
            this->vertices.push_back({
                vd,
                nd,
                d,
                {
                    {
                        tile.Tile[0],
                        this->tileUV(D, tile.getTileOrient(0), is256, uvOff) },
                    {
                        tile.Tile[1],
                        this->tileUV(D, tile.getTileOrient(1), is256, uvOff) },
                    {
                        tile.Tile[2],
                        this->tileUV(D, tile.getTileOrient(2), is256, uvOff)
                    }
                }
            });

            this->indexes.push_back(0);
            this->indexes.push_back(1);
            this->indexes.push_back(2);
            this->indexes.push_back(0);
            this->indexes.push_back(2);
            this->indexes.push_back(3);
        }
    }
}

void LandscapeManager::addZone(NL3D::CLandscape& landscape,
    const std::string& zoneSearchDirectory,
    const sint x, const sint y)
{
    std::string zoneFilename(zoneSearchDirectory);
    zoneFilename += zoneName(x, y);
    zoneFilename += ".zonel";

    NLMISC::CIFile zoneFile;
    if (zoneFile.open(zoneFilename))
    {
        nlinfo("Found Neighbor Zone: %s", zoneFilename.c_str());
        NL3D::CZone zone;
        zone.serial(zoneFile);
        landscape.addZone(zone);
        zoneFile.close();
        return;
    }

    zoneFilename = zoneSearchDirectory;
    zoneFilename += zoneNameLowerCase(x, y);
    zoneFilename += ".zonel";
    if (zoneFile.open(zoneFilename))
    {
        nlinfo("Found Neighbor Zone: %s", zoneFilename.c_str());
        NL3D::CZone zone;
        zone.serial(zoneFile);
        landscape.addZone(zone);
        zoneFile.close();
    }
}

void LandscapeManager::addNeighborZones(NL3D::CLandscape& landscape, const uint16& zoneId,
    const std::string& zoneSearchDirectory)
{
    const sint x(zoneId & 255);
    const sint y(zoneId >> 8);

    this->addZone(landscape, zoneSearchDirectory, x - 1, y - 1);
    this->addZone(landscape, zoneSearchDirectory, x + 0, y - 1);
    this->addZone(landscape, zoneSearchDirectory, x + 1, y - 1);
    this->addZone(landscape, zoneSearchDirectory, x - 1, y + 0);
    this->addZone(landscape, zoneSearchDirectory, x + 0, y + 0);
    this->addZone(landscape, zoneSearchDirectory, x + 1, y + 0);
    this->addZone(landscape, zoneSearchDirectory, x - 1, y + 1);
    this->addZone(landscape, zoneSearchDirectory, x + 0, y + 1);
    this->addZone(landscape, zoneSearchDirectory, x + 1, y + 1);
}

void LandscapeManager::loadTileBank(NL3D::CLandscape& landscape, const std::string& bankFilePath)
{
    if (!bankFilePath.empty())
    {
        NLMISC::CIFile bankFile(bankFilePath);
        auto &tileBank = landscape.TileBank;
        tileBank.serial(bankFile);
        nldebug("TileBank land count %i", tileBank.getLandCount());
        nldebug("TileBank tileSet count %i", tileBank.getTileSetCount());
        nldebug("TileBank tile count %i", tileBank.getTileCount());
    }
}

void LandscapeManager::createBuffer(
    SDL_GPUDevice* device, const void* data, size_t size,
    SDL_GPUBufferUsageFlags usage, SDL_GPUBuffer* buffer)
{
    SDL_GPUBufferCreateInfo bufferCreateInfo = SDL_GPUBufferCreateInfo();
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.size = static_cast<Uint32>(size);
    buffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
    
    SDL_GPUTransferBufferCreateInfo transferInfo = SDL_GPUTransferBufferCreateInfo();
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(size);
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    memcpy(transferData, data, size);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
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
