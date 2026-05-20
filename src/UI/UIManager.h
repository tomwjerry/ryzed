#pragma once
#include "imgui.h"
#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <string>

class UIManager
{
private:
    ImGuiIO* io;
    std::function<void(const std::string& path)> landscapeFileSelCB = nullptr;

public:
    UIManager(ImGuiIO* inio);
    ~UIManager();
    void Render();

    void RegisterLandscapeFileSelCB(std::function<void(const std::string& path)> cb);
};
