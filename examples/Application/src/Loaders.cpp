
#include "Loaders.h"

#include "Application.h"
#include "Resources.h"

#include "Teide/Buffer.h"

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fmt/format.h>

MeshWithBounds LoadMesh(const char* filename)
{
    Assimp::Importer importer;

    const auto importFlags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
        | aiProcess_SortByPType | aiProcess_RemoveComponent | aiProcess_RemoveRedundantMaterials
        | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_ConvertToLeftHanded | aiProcess_FlipWindingOrder;

    const aiScene* scene = importer.ReadFile(filename, importFlags);
    if (!scene)
    {
        throw ApplicationError(importer.GetErrorString());
    }

    if (scene->mNumMeshes == 0)
    {
        throw ApplicationError(fmt::format("Model file '{}' contains no meshes", filename));
    }
    if (scene->mNumMeshes > 1)
    {
        throw ApplicationError(fmt::format("Model file '{}' contains too many meshes", filename));
    }

    const aiMesh& mesh = **scene->mMeshes;

    MeshWithBounds ret;
    ret.mesh.vertexData.reserve(mesh.mNumVertices * sizeof(Vertex));

    for (unsigned int i = 0; i < mesh.mNumVertices; i++)
    {
        const auto pos = mesh.mVertices[i];
        const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D{};
        const auto norm = mesh.HasNormals() ? mesh.mNormals[i] : aiVector3D{};
        const auto color = mesh.HasVertexColors(0) ? mesh.mColors[0][i] : aiColor4D{1, 1, 1, 1};

        const Vertex vertex = {
            .position = {pos.x, pos.y, pos.z},
            .texCoord = {uv.x, uv.y},
            .normal = {norm.x, norm.y, norm.z},
            .color = {color.r, color.g, color.b},
        };
        Teide::AppendBytes(ret.mesh.vertexData, vertex);

        ret.aabb.Add(vertex.position);
    }

    ret.mesh.indexData.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3 * sizeof(std::uint16_t));

    for (unsigned int i = 0; i < mesh.mNumFaces; i++)
    {
        const auto& face = mesh.mFaces[i];
        assert(face.mNumIndices == 3);
        for (int j = 0; j < 3; j++)
        {
            if (face.mIndices[j] > std::numeric_limits<uint16_t>::max())
            {
                throw ApplicationError("Too many vertices for 16-bit indices");
            }
            Teide::AppendBytes(ret.mesh.indexData, static_cast<uint16_t>(face.mIndices[j]));
        }
    }

    return ret;
}
