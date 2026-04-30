#pragma once
#include "imgui.h"
#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

class UIManager
{
private:
    ImGuiIO* io;

public:
    UIManager(ImGuiIO* inio);
    ~UIManager();
    void Render();
};
