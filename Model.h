#pragma once
#include <vector>
#include "Mesh.h"
#include "Material.h"

class Model
{
public:
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    // Load model from glTF file
    void Load(const char* assetFileName);
    // Render the entire model
    void Render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS materialBufferAddress) const;
    
   
};