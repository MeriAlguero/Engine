#pragma once
#include "Module.h"
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <cstdint>

class ModuleD3D12 : public Module
{
private:
    HWND hWnd = NULL;
    uint32_t windowWidth = 0;
    uint32_t windowHeight = 0;

    // Core DX12
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    Microsoft::WRL::ComPtr<ID3D12Device5> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    // Swap chain y render targets
    static constexpr uint32_t BACK_BUFFER_COUNT = 2;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    uint32_t currentFrame = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    uint32_t rtvDescriptorSize = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[BACK_BUFFER_COUNT];

    // Depth stencil
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
    UINT dsvDescriptorSize = 0;

    // Fence
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;

public:
    ModuleD3D12(HWND hWnd);
    ~ModuleD3D12();

    uint32_t getWindowWidth() const { return windowWidth; }
    uint32_t getWindowHeight() const { return windowHeight; }

    bool init() override;
    void preRender() override;
    void postRender() override;
    void resize(uint32_t width, uint32_t height);
    bool flush();

    // Getters para otros módulos
    ID3D12Device5* getDevice() const;
    uint32_t getCurrentFrame() const;
    UINT getLastCompletedFrame() const;
    ID3D12GraphicsCommandList* getCommandList() const;
    ID3D12CommandAllocator* getCommandAllocator() const;
    ID3D12CommandQueue* getDrawCommandQueue() const;
    ID3D12Resource* getBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor() const {
        return dsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

private:
    void enableDebugLayer();
    bool createFactory();
    bool createDevice(bool useWarp);
};