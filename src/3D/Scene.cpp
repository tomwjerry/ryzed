#include "Scene.h"

/**
 * MIT License, Based on code with Copyright (c) 2025 Loïc Chen
 */

void Scene::AddNode(Node* node)
{
    nodes.push_back(node);
}

void Scene::PrepareForRender(SDL_GPUDevice* device)
{

}
