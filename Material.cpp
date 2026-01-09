#include "Globals.h"
#include "Material.h"
#include "Application.h"
#include "ModuleResource.h"
#include "gltf_utils.h" 

void Material::Load(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, const char* basePath)
{
    ModuleResource* resources = app->getResources();

    //LOAD BASE COLOR
    if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() >= 4)
    {
        baseColor = DirectX::XMFLOAT4(
            (float)gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
            (float)gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
            (float)gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
            (float)gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]
        );
    }
    else
    {
        //Default color
        baseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    //LOAD BASE COLOR TEXTURE
    if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
    {
        const tinygltf::Texture& tex = model.textures[gltfMaterial.pbrMetallicRoughness.baseColorTexture.index];
        if (tex.source >= 0 && tex.source < model.images.size())
        {
            const tinygltf::Image& image = model.images[tex.source];

            if (!image.uri.empty())
            {
                // Build full texture path
                std::string texturePath = std::string(basePath) + image.uri;

                // Create texture from file
                texture = resources->createTextureFromFile(texturePath, false);

                if (texture)
                {
                    LOG("Loaded texture: %s", texturePath.c_str());
                    
                    srvIndex = 0;
                }
                else
                {
                    LOG("Failed to load texture: %s", texturePath.c_str());
                }
            }
        }
    }
    else
    {
        // No texture
        texture = nullptr;
        srvIndex = 0; // Will use null descriptor
    }
}