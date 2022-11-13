
#pragma once

#include "GeoLib/Box.h"
#include "Teide/Mesh.h"
#include "Teide/Texture.h"

#include <string_view>

struct MeshWithBounds
{
    Teide::MeshData mesh;
    Geo::Box3 aabb;
};

MeshWithBounds LoadMesh(const char* filename);
