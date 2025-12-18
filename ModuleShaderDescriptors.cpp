#include "Globals.h"
#include "ModuleShaderDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"

bool ModuleShaderDescriptors::init() {
    auto device = app->getD3D12()->getDevice();
    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = MAX_DESCRIPTORS;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    return SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
}

DescriptorTable ModuleShaderDescriptors::allocTable(UINT size) {
    DescriptorTable table(app->getD3D12()->getDevice(), heap.Get(), currentDescriptorIndex, descriptorSize);
    currentDescriptorIndex += size;
    return table;
}

DescriptorTable::DescriptorTable(ID3D12Device* dev, ID3D12DescriptorHeap* h, UINT idx, UINT size)
    : device(dev), heap(h), index(idx), descriptorSize(size) {}

void DescriptorTable::createTextureSRV(ID3D12Resource* texture) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;

    device->CreateShaderResourceView(texture, &srvDesc, handle);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable::getGPUHandle() const {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;
    return handle;
}