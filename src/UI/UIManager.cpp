#include "UIManager.h";

UIManager::UIManager(ImGuiIO* inio)
{
    this->io = inio;
    NFD_Init();
}

UIManager::~UIManager()
{
    NFD_Quit();
}

void UIManager::Render()
{
    static float f = 0.0f;
    static int counter = 0;

    // Create a window and append into it.
    ImGui::Begin("RyzEd");

    if (ImGui::Button("Load NeL file"))
    {
        nfdu8char_t *outPath;
        nfdopendialogu8args_t args = {0};
        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
        if (result == NFD_OKAY)
        {
            
            NFD_FreePathU8(outPath);
        }
    }

    ImGui::Text(
        "Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / this->io->Framerate, this->io->Framerate);
    ImGui::End();
}
