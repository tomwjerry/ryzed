#include "LandscapeManager.h"
#include <istream>
#include <fstream>
#include "nel/ligo/primitive_utils.h"
#include "nel/misc/path.h"
#include "nel/misc/config_file.h"
#include "nel/misc/file.h"
#include "nel/misc/i_xml.h"

LandscapeManager::LandscapeManager(const std::string& cfgFile)
{
    NLMISC::CPath::addSearchPath("D:/ryzed/build/Debug");
    NLMISC::CPath::addSearchPath("D:/ryzomcore-quickstart-4.0-pre3/leveldesign/primitives");
	this->ligoConfig = new NLLIGO::CLigoConfig();
    NLLIGO::CPrimitiveContext::instance().CurrentLigoConfig = this->ligoConfig;
    this->editLandscapePrimitive = new NLLIGO::CPrimitives();
    this->ligoConfig->readPrimitiveClass(cfgFile, false);
	NLLIGO::CPrimitiveContext::instance().CurrentPrimitive = this->editLandscapePrimitive;
}

LandscapeManager::~LandscapeManager()
{
	delete this->ligoConfig;
	delete this->editLandscapePrimitive;
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
        if (parseText.find("landscapePrimitive") != std::string::npos)
        {
            // Found the landscape section
            // Include all after ":" until end of line or comma
            this->parsePath(parseText);
            this->editLandscapePrimitivePath = parseText;
        }
        else if (parseText.find("land") != std::string::npos)
        {
            this->parsePath(parseText);
            this->editZoneRegionPath = parseText;
        }
    }

    // Close the file
    file.close();

    // Load the landscape primitive
    if (!this->editLandscapePrimitivePath.empty())
    {
        this->LoadLandscapePrimitive(editLandscapePrimitivePath);
    }

    if (!this->editZoneRegionPath.empty())
    {
        this->LoadZoneRegion(editZoneRegionPath);
    }
}

void LandscapeManager::parsePath(std::string& path)
{
    path = path.substr(path.find(":") + 1);
    path.erase(path.find_last_not_of(" \n\r\t\,\"") + 1);
}

void LandscapeManager::LoadLandscapePrimitive(const std::string& path)
{
    this->editLandscapePrimitivePath = path;
    NLMISC::CIFile fileIn;
    if (fileIn.open(path))
    {
        NLMISC::CIXml xml;
        xml.init(fileIn);
        // Get the root node
        NLLIGO::loadXmlPrimitiveFile(*this->editLandscapePrimitive, path.c_str(), *this->ligoConfig);
    }
}

void LandscapeManager::LoadZoneRegion(const std::string& path)
{
    this->editZoneRegionPath = path;
    NLMISC::CIFile fileIn;
    if (fileIn.open(path))
    {
        NLMISC::CIXml xml;
        xml.init(fileIn);
        // Get the root node
        editZoneRegion.serial(xml);
    }
}

void LandscapeManager::Save(const std::string& path)
{
    this->editLandscapePath = path;
}
void LandscapeManager::Save()
{
    this->SaveLandscapePrimitive();
    this->SaveZoneRegion();
}

void LandscapeManager::SaveLandscapePrimitive()
{

}
void LandscapeManager::SaveLandscapePrimitive(const std::string& path)
{
    this->editLandscapePrimitivePath = path;
    this->SaveLandscapePrimitive();
}

void LandscapeManager::SaveZoneRegion()
{

}
void LandscapeManager::SaveZoneRegion(const std::string& path)
{
    this->editZoneRegionPath = path;
    this->SaveZoneRegion();
}
