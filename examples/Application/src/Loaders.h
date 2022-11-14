
#pragma once

#include "GeoLib/Box.h"
#include "Teide/Mesh.h"
#include "Teide/Texture.h"

#include <string_view>

Teide::MeshData LoadMesh(const char* filename);
Teide::TextureData LoadTexture(const char* filename);
