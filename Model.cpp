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

void Model::Render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS materialBufferAddress) const
{
    if (meshes.empty())
        return;
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 
    for (const auto& mesh : meshes)
    {
        //Debug Print
        char debugBuf[256];
       
        sprintf_s(debugBuf, "Rendering Mesh: Verts %u, Indices %u, MatIdx %u\n",
            mesh.getVertexCount(), mesh.getIndexCount(), mesh.getMaterialIndex());
        OutputDebugStringA(debugBuf);

        // Material Binding (Placeholder)
        UINT matIdx = mesh.getMaterialIndex();
        if (matIdx < materials.size())
        {
            const Material& mat = materials[matIdx];
            commandList->SetGraphicsRootConstantBufferView(1, materialBufferAddress);
            if (mat.texture != nullptr)
            {
            }
        }
        if (materials[matIdx].texture != nullptr)
        {
            //commandList->SetGraphicsRootDescriptorTable(2, materials[matIdx].getSRVHandle());
        }

        // Bind Buffers
        mesh.BindBuffers(commandList);

        //Draw
        if (mesh.getIndexCount() > 0) // Check if indices exist
        {
            commandList->DrawIndexedInstanced(mesh.getIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(mesh.getVertexCount(), 1, 0, 0);
        }
    }
}

