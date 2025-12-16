#pragma once

#include "Module.h"

#include <filesystem>
#include <vector>

namespace DirectX 
{ 
    class ScratchImage;  
    struct TexMetadata; }


class ModuleResource : public Module
{
private:
    // Comandament i Allocator dedicats a l'inicialització de recursos
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    // dades temporals per a la càrrega de textures (note: multithreading issues)
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
    std::vector<UINT> numRows;
    std::vector<UINT64> rowSizes;

    // Estructura per a l'alliberament segur de recursos
    struct DeferredFree
    {
        UINT frame = 0;
        ComPtr<ID3D12Resource> resource;
    };

    std::vector<DeferredFree> deferredFrees;

public:

    ModuleResource();
    ~ModuleResource();

    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    // Creació de Buffers
    ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size, const char* name);
    ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);

    // Creació de Textures
    ComPtr<ID3D12Resource> createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format);
    ComPtr<ID3D12Resource> createTextureFromMemory(const void* data, size_t size, const char* name);
    ComPtr<ID3D12Resource> createTextureFromFile(const std::filesystem::path& path, bool defaultSRGB = false);

    // Creació de Render Targets (RT) i Depth Stencils (DS)
    ComPtr<ID3D12Resource> createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, UINT sampleCount, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> createDepthStencil(DXGI_FORMAT format, size_t width, size_t height, UINT sampleCount, float clearDepth, uint8_t clearStencil, const char* name);

    // Helpers per Cubemaps
    ComPtr<ID3D12Resource> createCubemapRenderTarget(DXGI_FORMAT format, size_t size, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> createCubemapRenderTarget(DXGI_FORMAT format, size_t size, size_t mipLevels, const Vector4& clearColour, const char* name);

    // Gestió de Recursos
    void deferRelease(ComPtr<ID3D12Resource> resource);

private:
    // Funcions de suport privades
    ComPtr<ID3D12Resource> createTextureFromImage(const ScratchImage& image, const char* name);
    ComPtr<ID3D12Resource> createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, size_t arraySize, size_t mipLevels, UINT sampleCount, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> getUploadHeap(size_t size);

};

// --- Implementacions Inline ---
inline ComPtr<ID3D12Resource> ModuleResource::createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, UINT sampleCount, const Vector4& clearColour, const char* name)
{
    return createRenderTarget(format, width, height, 1, 1, sampleCount, clearColour, name);
}

inline ComPtr<ID3D12Resource> ModuleResource::createCubemapRenderTarget(DXGI_FORMAT format, size_t size, const Vector4& clearColour, const char* name)
{
    return createRenderTarget(format, size, size, 6, 1, 1, clearColour, name);
}

inline ComPtr<ID3D12Resource> ModuleResource::createCubemapRenderTarget(DXGI_FORMAT format, size_t size, size_t mipLevels, const Vector4& clearColour, const char* name)
{
    return createRenderTarget(format, size, size, 6, mipLevels, 1, clearColour, name);
}