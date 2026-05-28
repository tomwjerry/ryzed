/** 
 * Based on the code by zerotacg
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

void LandscapeManager::Load(const std::string& path)
{
    // A landscape is a .landscape.json file
    // Open the file
    std::ifstream file(path);
    if (!file.is_open())
    {
        return;
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
}

void LandscapeManager::parsePath(std::string& path)
{
    path = path.substr(path.find(":") + 1);
    path.erase(path.find_last_not_of(" \n\r\t\,\"") + 1);
}

void LandscapeManager::LoadZoneDir(const std::string& path);
{
    for (const auto& entry : fs::directory_iterator(path))
    {
        this->LoadZone(entry.path().string());
    }
}

void LandscapeManager::LoadZone(const std::string& path)
{
    CIFile zoneFile;
    if (!zoneFile.open(path))
    {
        nlwarning("Can't open the file for reading: %s", path.c_str());
        return;
    }
    
    CLandscape landscape;
    CAABBox bbox;
    CZone loadingZone;
    loadingZone.serial(zoneFile);
    zoneFile.close();

    const auto zoneId(loadingZone.getZoneId());
    landscape.setNoiseMode(false);
    // add neighbor zones to get the same border vertices
    addNeighborZones(landscape, zoneId, zoneSearchDirectory);
    auto zone = landscape.getZone(zoneId);
    if (zone == nullptr)
    {
        nlerror("Can't find zone with id: %i", zoneId);
        return EXIT_FAILURE;
    }
    try
    {
        this->loadTileBank(landscape, bankFilePath);
    }
    catch (const Exception &)
    {
        nlerror("Can't load bankfile: %s", bankFilePath.c_str());
        return EXIT_FAILURE;
    }

    const sint zoneX(zoneId & 255);
    const sint zoneY(zoneId >> 8);
    CVector zoneOffset(160.0f * zoneX, -160.0f * zoneY, 0.0f);
    QImage normalMap = this->createNormalMap(NORMAL_MAP_SIZE, NORMAL_MAP_SIZE);
    QImage tileIdMaps[TILE_LAYER_COUNT] = {
        createTileIdMap(TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE),
        createTileIdMap(TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE),
        createTileIdMap(TILE_ID_MAP_SIZE, TILE_ID_MAP_SIZE)
    };
    for (sint patchIndex = 0; patchIndex < zone->getNumPatchs(); patchIndex++)
    {
        buildFaces(landscape, zoneId, patchIndex, output, tileIdMaps, normalMap);
    }
}

uint8 LandscapeManager::getPatchTileIndex(const CPatch &patch, const uint8 s, const uint8 t)
{
    return t * patch.getOrderS() + s;
}

// check CTessFace::initTileUvRGBA for correct calculation
CUV LandscapeManager::tileOrientation(CUV in, uint8 orientation)
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

CUV LandscapeManager::tileUV(const CUV &in, uint8 orientation, bool is256, uint8 uvOff)
{
    CUV out(in);
    if (is256)
    {
        out *= 0.5;
        // with rotation applied afterward we need to reverse the already applied rotation in uvOff
        uvOff = (uvOff + orientation) & 3;
        if (uvOff == 2 || uvOff == 3)
            out.U += 0.5;
        if (uvOff == 1 || uvOff == 2)
            out.V += 0.5;
    }
    // Do the HalfPixel scale bias.
    float hBiasXY, hBiasZ;
    if (is256)
    {
        hBiasXY = CLandscapeGlobals::TilePixelBias256;
        hBiasZ = CLandscapeGlobals::TilePixelScale256;
    }
    else
    {
        hBiasXY = CLandscapeGlobals::TilePixelBias128;
        hBiasZ = CLandscapeGlobals::TilePixelScale128;
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

void LandscapeManager::setPixel(QImage &image, int x, int y, uint16 grayscale)
{
    nlassert(image.format() == QImage::Format_Grayscale16);
    ((uint16 *)image.scanLine(y))[x] = grayscale;
}

QImage LandscapeManager::createNormalMap(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(QColor::fromRgbF(0.0f, 0.0f, 1.0f));
    return image;
}

void LandscapeManager::drawNormalMap(const CPatch &patch, QImage &image)
{
    CBezierPatch bezierPatch;
    patch.unpack(bezierPatch);
    auto scale = image.width() / PATCH_SIZE;
    auto orderS = patch.getOrderS() * scale;
    auto orderT = patch.getOrderT() * scale;
    float OOS = 1.0f / (orderS - 1);
    float OOT = 1.0f / (orderT - 1);
    for (auto y = 0; y < orderT; y++)
    {
        for (auto x = 0; x < orderS; x++)
        {
            CVector normal(bezierPatch.evalNormal(x * OOS, y * OOT));
            image.setPixelColor(x, y, QColor::fromRgbF(normal.x, normal.y, normal.z));
        }
    }
}

void LandscapeManager::drawImage(QImage &target, int x, int y, QImage &part)
{
    QPainter painter(&target);
    painter.drawImage(QPoint(x, y), part);
}

QImage LandscapeManager::createTileIdMap(int width, int height)
{
    QImage image(width, height, QImage::Format_RGBA64);
    image.fill(QColor::fromRgba64(qRgba64(NL_TILE_ELM_LAYER_EMPTY, 0, 0, NL_TILE_ELM_LAYER_EMPTY)));
    return image;
}

uint8 LandscapeManager::getTileOrientation(
    const CPatch &patch, const CTileElement &tile, const uint8 layer)
{

    return tile.getTileOrient(layer);
}

void LandscapeManager::drawTileInfoMap(const CPatch &patch, QImage &image, uint8 layer)
{
    const auto &tiles = patch.Tiles;
    auto &tileBank = patch.getLandscape()->TileBank;

    for (auto y = 0; y < patch.getOrderT(); y++)
    {
        for (auto x = 0; x < patch.getOrderS(); x++)
        {
            auto tileIndex = getPatchTileIndex(patch, x, y);
            const auto &tile = tiles[tileIndex];
            const auto orientation = tile.getTileOrient(layer);
            const auto tileId = tile.Tile[layer];
            uint8 rotAlpha = 0;
            if (tileId != NL_TILE_ELM_LAYER_EMPTY)
            {
                rotAlpha = tileBank.getTile(tileId)->getRotAlpha();
            }
            image.setPixelColor(x, y,
                QColor::fromRgba64(qRgba64(tileId, orientation, rotAlpha, NL_TILE_ELM_LAYER_EMPTY)));
        }
    }
}

void LandscapeManager::buildFaces(
    CLandscape &landscape, sint zoneId, sint patch,
    OutputData &output, QImage *tileIdMap, QImage &normalMap)
{
    CUV A(0, 0), B(0, 1), C(1, 1), D(1, 0);
    CZone *pZone = landscape.getZone(zoneId);

    // Then trace all patch.
    nlassert(patch >= 0);
    nlassert(patch < pZone->getNumPatchs());
    const CPatch *pa = const_cast<const CZone *>(pZone)->getPatch(patch);
    const auto &tiles = pa->Tiles;
    CBezierPatch bezierPatch;
    pa->unpack(bezierPatch);

    // Build the faces.
    //=================
    uint8 ordS = pa->getOrderS();
    uint8 ordT = pa->getOrderT();
    uint16 patchOffset(patch * PATCH_SIZE);
    uint8 x, y;
    float pixelOffset = 0.125f / TILE_ID_MAP_SIZE;
    float OOS = 1.0f / ordS;
    float OOT = 1.0f / ordT;

    uint16 normal_offset_x((patch * NORMAL_SIZE) % normalMap.width()),
        normal_offset_y(((patch * NORMAL_SIZE) / normalMap.height()) * NORMAL_SIZE);
    QImage normalMapPatch = createNormalMap(NORMAL_SIZE, NORMAL_SIZE);
    drawNormalMap(*pa, normalMapPatch);
    drawImage(normalMap, normal_offset_x, normal_offset_y, normalMapPatch);

    uint16 offset_x(patchOffset % TILE_ID_MAP_SIZE),
        offset_y((patchOffset / TILE_ID_MAP_SIZE) * PATCH_SIZE);
    QImage tileInfoMapPatch = createTileIdMap(PATCH_SIZE, PATCH_SIZE);
    drawTileInfoMap(*pa, tileInfoMapPatch, 0);
    drawImage(tileIdMap[0], offset_x, offset_y, tileInfoMapPatch);
    drawTileInfoMap(*pa, tileInfoMapPatch, 1);
    drawImage(tileIdMap[1], offset_x, offset_y, tileInfoMapPatch);
    drawTileInfoMap(*pa, tileInfoMapPatch, 2);
    drawImage(tileIdMap[2], offset_x, offset_y, tileInfoMapPatch);

    for (y = 0; y < ordT; y++)
    {
        for (x = 0; x < ordS; x++)
        {
            auto tileIndex = getPatchTileIndex(*pa, x, y);
            const auto &tile = tiles[tileIndex];
            if (tile.Tile[0] == NL_TILE_ELM_LAYER_EMPTY)
            {
                nlwarning("tile base layer not defined patch %d x %d y %d tileIndex %d",
                    patch, x, y, tileIndex);
            }
            uint16 imageX = offset_x + x;
            uint16 imageY = offset_y + y;
            CUV tileInfo(imageX, imageY);
            CUV b(tileInfo.U, tileInfo.V + pixelOffset);
            CUV c(tileInfo.U + pixelOffset, tileInfo.V + pixelOffset);
            CUV d(tileInfo.U + pixelOffset, tileInfo.V);
            CVector uvScaleBias;
            bool is256;
            uint8 uvOff;
            tile.getTile256Info(is256, uvOff);

            CVector va(pa->computeContinousVertex(x * OOS, y * OOT));
            CVector vb(pa->computeContinousVertex(x * OOS, (y + 1) * OOT));
            CVector vc(pa->computeContinousVertex((x + 1) * OOS, (y + 1) * OOT));
            CVector vd(pa->computeContinousVertex((x + 1) * OOS, y * OOT));
            CVector na(bezierPatch.evalNormal(x * OOS, y * OOT));
            CVector nb(bezierPatch.evalNormal(x * OOS, (y + 1) * OOT));
            CVector nc(bezierPatch.evalNormal((x + 1) * OOS, (y + 1) * OOT));
            CVector nd(bezierPatch.evalNormal((x + 1) * OOS, y * OOT));

            output.vertices.push_back({ .position = va,
                .normal = na,
                .tileInfoUv = a,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(A, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(A, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(A, tile.getTileOrient(2), is256, uvOff) } } });
            output.vertices.push_back({ .position = vb,
                .normal = nb,
                .tileInfoUv = b,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(B, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(B, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(B, tile.getTileOrient(2), is256, uvOff) } } });
            output.vertices.push_back({ .position = vc,
                .normal = nc,
                .tileInfoUv = c,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(C, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(C, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(C, tile.getTileOrient(2), is256, uvOff) } } });

            output.vertices.push_back({ .position = va,
                .normal = na,
                .tileInfoUv = a,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(A, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(A, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(A, tile.getTileOrient(2), is256, uvOff) } } });
            output.vertices.push_back({ .position = vc,
                .normal = nc,
                .tileInfoUv = c,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(C, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(C, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(C, tile.getTileOrient(2), is256, uvOff) } } });
            output.vertices.push_back({ .position = vd,
                .normal = nd,
                .tileInfoUv = d,
                .tile = {
                    { .tileId = tile.Tile[0], .uv = tileUV(D, tile.getTileOrient(0), is256, uvOff) },
                    { .tileId = tile.Tile[1], .uv = tileUV(D, tile.getTileOrient(1), is256, uvOff) },
                    { .tileId = tile.Tile[2], .uv = tileUV(D, tile.getTileOrient(2), is256, uvOff) } } });
        }
    }
}

std::string LandscapeManager::getLongArgFirstValue(
    const NLMISC::CCmdArgs &args, const std::string &argName)
{
    std::string firstValue;
    const auto values = args.getLongArg(argName);
    if (!values.empty())
    {
        firstValue = values.front();
    }
    return firstValue;
}

void LandscapeManager::addZone(CLandscape &landscape,
    const std::string &zoneSearchDirectory,
    const sint x, const sint y)
{
    std::string zoneFilename(zoneSearchDirectory);
    zoneFilename += zoneName(x, y);
    zoneFilename += ".zonel";

    CIFile zoneFile;
    if (zoneFile.open(zoneFilename))
    {
        nlinfo("Found Neighbor Zone: %s", zoneFilename.c_str());
        CZone zone;
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
        CZone zone;
        zone.serial(zoneFile);
        landscape.addZone(zone);
        zoneFile.close();
    }
}

void LandscapeManager::addNeighborZones(CLandscape &landscape, const uint16 &zoneId,
    const std::string &zoneSearchDirectory)
{
    const sint x(zoneId & 255);
    const sint y(zoneId >> 8);

    addZone(landscape, zoneSearchDirectory, x - 1, y - 1);
    addZone(landscape, zoneSearchDirectory, x + 0, y - 1);
    addZone(landscape, zoneSearchDirectory, x + 1, y - 1);
    addZone(landscape, zoneSearchDirectory, x - 1, y + 0);
    addZone(landscape, zoneSearchDirectory, x + 0, y + 0);
    addZone(landscape, zoneSearchDirectory, x + 1, y + 0);
    addZone(landscape, zoneSearchDirectory, x - 1, y + 1);
    addZone(landscape, zoneSearchDirectory, x + 0, y + 1);
    addZone(landscape, zoneSearchDirectory, x + 1, y + 1);
}

std::optional<std::string> LandscapeManager::openFile(COFile &file,
    const std::string &directory, const std::string &basename, const std::string &suffix)
{
    std::string fileName = basename + suffix;
    std::string filePath = directory + "/" + fileName;
    if (!file.open(filePath, false, false, false))
    {
        nlwarning("Can't open the file for writing: %s", filePath.c_str());
    }

    return fileName;
}

void LandscapeManager::loadTileBank(CLandscape &landscape, const std::string &bankFilePath)
{
    if (!bankFilePath.empty())
    {
        CIFile bankFile(bankFilePath);
        auto &tileBank = landscape.TileBank;
        tileBank.serial(bankFile);
        nldebug("TileBank land count %i", tileBank.getLandCount());
        nldebug("TileBank tileSet count %i", tileBank.getTileSetCount());
        nldebug("TileBank tile count %i", tileBank.getTileCount());
    }
}
