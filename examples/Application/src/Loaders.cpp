
#include "Loaders.h"

#include "Application.h"
#include "Resources.h"

#include "Teide/Buffer.h"

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <format>
#include <stb_image.h>

Teide::MeshData LoadMesh(const char* filename)
{
    using namespace Teide::BasicTypes;

    Assimp::Importer importer;

    const auto importFlags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
        | aiProcess_SortByPType | aiProcess_RemoveComponent | aiProcess_RemoveRedundantMaterials
        | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_ConvertToLeftHanded;

    const aiScene* scene = importer.ReadFile(filename, importFlags);
    if (!scene)
    {
        throw ApplicationError(importer.GetErrorString());
    }

    if (scene->mNumMeshes == 0)
    {
        throw ApplicationError(std::format("Model file '{}' contains no meshes", filename));
    }
    if (scene->mNumMeshes > 1)
    {
        throw ApplicationError(std::format("Model file '{}' contains too many meshes", filename));
    }

    const aiMesh& mesh = **scene->mMeshes;

    Teide::MeshData ret;
    ret.vertexLayout = VertexLayoutDesc;
    ret.vertexData.reserve(mesh.mNumVertices * sizeof(Vertex));

    for (unsigned int i = 0; i < mesh.mNumVertices; i++)
    {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto pos = mesh.mVertices[i];
        const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D{};
        const auto norm = mesh.HasNormals() ? mesh.mNormals[i] : aiVector3D{};
        const auto color = mesh.HasVertexColors(0) ? mesh.mColors[0][i] : aiColor4D{1, 1, 1, 1};
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        const Vertex vertex = {
            .position = {pos.x, pos.y, pos.z},
            .texCoord = {uv.x, uv.y},
            .normal = {norm.x, norm.y, norm.z},
            .color = {color.r, color.g, color.b},
        };
        Teide::AppendBytes(ret.vertexData, vertex);

        ret.aabb.Add(vertex.position);
    }

    ret.indexData.reserve(static_cast<usize>(mesh.mNumFaces) * 3 * sizeof(uint16));

    for (const auto& face : std::span(mesh.mFaces, mesh.mNumFaces))
    {
        assert(face.mNumIndices == 3u);
        for (const int index : std::span(face.mIndices, face.mNumIndices))
        {
            if (index > std::numeric_limits<uint16>::max())
            {
                throw ApplicationError("Too many vertices for 16-bit indices");
            }
            Teide::AppendBytes(ret.indexData, static_cast<uint16>(index));
        }
    }

    return ret;
}

Teide::TextureData LoadTexture(const char* filename)
{
    if (filename)
    {
        struct StbiDeleter
        {
            void operator()(stbi_uc* p) { stbi_image_free(p); }
        };
        using StbiPtr = std::unique_ptr<stbi_uc, StbiDeleter>;

        // Load image
        int width{}, height{}, channels{};
        const auto pixels = StbiPtr(stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
        if (!pixels)
        {
            throw ApplicationError(std::format("Error loading texture '{}'", filename));
        }

        const auto imageSize = static_cast<std::size_t>(width) * height * 4;

        return {
            .size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
            .format = Teide::Format::Byte4Srgb,
            .mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1,
            .samplerState = {.magFilter = Teide::Filter::Linear, .minFilter = Teide::Filter::Linear},
            .pixels = Teide::ToBytes(std::span(pixels.get(), imageSize)),
        };
    }
    else
    {
        // Create default checkerboard texture
        constexpr auto c0 = std::uint32_t{0xffffffff};
        constexpr auto c1 = std::uint32_t{0xffff00ff};
        constexpr auto pixels = std::array{
            c0, c1, c0, c1, c0, c1, c0, c1, //
            c1, c0, c1, c0, c1, c0, c1, c0, //
            c0, c1, c0, c1, c0, c1, c0, c1, //
            c1, c0, c1, c0, c1, c0, c1, c0, //
            c0, c1, c0, c1, c0, c1, c0, c1, //
            c1, c0, c1, c0, c1, c0, c1, c0, //
            c0, c1, c0, c1, c0, c1, c0, c1, //
            c1, c0, c1, c0, c1, c0, c1, c0,
        };
        return {
            .size = {8, 8},
            .format = Teide::Format::Byte4Srgb,
            .mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(8))) + 1,
            .samplerState = {.magFilter = Teide::Filter::Nearest, .minFilter = Teide::Filter::Nearest},
            .pixels = Teide::ToBytes(pixels),
        };
    }
}
