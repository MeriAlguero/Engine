#include "Globals.h"
#include "Model.h"
#include "gltf_utils.h" 
#include <filesystem>



void Model::Load(const char* assetFileName)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;
    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, assetFileName);

    if (loadOk)
    {
        std::filesystem::path path(assetFileName);
        std::string basePath = path.parent_path().string() + "/";

        // Load materials first
        for (const auto& gltfMaterial : model.materials)
        {
            Material material;
            material.Load(model, gltfMaterial, basePath.c_str());
            materials.push_back(material);
        }

        // If no materials, create a default one
        if (materials.empty())
        {
            Material defaultMat;
            defaultMat.baseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            materials.push_back(defaultMat);
        }

        // Load meshes
        for (const auto& gltfMesh : model.meshes)
        {
            for (const auto& primitive : gltfMesh.primitives)
            {
                Mesh mesh;
                mesh.Load(model, primitive);
                meshes.push_back(mesh);
            }
        }

        LOG("Loaded model %s: %zu meshes, %zu materials",
            assetFileName, meshes.size(), materials.size());
    }
    else
    {
        LOG("Error loading %s: %s", assetFileName, error.c_str());
    }
}

void Model::Render(ID3D12GraphicsCommandList* commandList) const
{
    if (meshes.empty())
        return;

    // Bind the vertex input layout
    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Render each mesh
    for (const auto& mesh : meshes)
    {
        // Bind mesh buffers
        mesh.BindBuffers(commandList);

        // TODO: Bind material here
        // - Bind material CBV (with baseColor and hasTexture flag)
        // - Bind texture SRV (or null descriptor)

        // Draw the mesh
        mesh.Draw(commandList);
    }
}
