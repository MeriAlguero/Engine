#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "ModuleShaderDescriptors.h"
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>

class ModuleD3D12 : public Module
{
private:
    /*WINDOW*/
    HWND hWnd = NULL;
    uint32_t windowWidth = 0;
    uint32_t windowHeight = 0;
    /*Core DX12*/
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter4> adapter;
    ComPtr<ID3D12Device5> device;


    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    //SWAP CHAIN Y RTV HEAP
    static constexpr uint32_t BACK_BUFFER_COUNT = 2;
    ComPtr<IDXGISwapChain4> swapChain;
    uint32_t currentFrame = 0;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    uint32_t rtvDescriptorSize = 0;

    ComPtr<ID3D12Resource> renderTargets[BACK_BUFFER_COUNT];

    ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent;


    //GEOMETRY
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    ComPtr<ID3D12Resource> bufferUploadHeap; //(Staging Buffer)
    //PIPELINE
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
    uint32_t vertexCount = 0;

    ComPtr<ID3D12Resource> textureDog;
    DescriptorTable dogDescriptor;

    ComPtr<ID3D12Resource> depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    UINT dsvDescriptorSize = 0;

    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12DescriptorHeap> samplerHeap;
    UINT srvDescriptorSize = 0;
    UINT samplerDescriptorSize = 0;

public:

    ModuleD3D12(HWND hWnd);
    ~ModuleD3D12();

    uint32_t getWindowWidth() const { return windowWidth; }
    uint32_t getWindowHeight() const { return windowHeight; }

    bool init() override;
    void preRender() override;
    void render() override;
    void postRender() override;
    void resize(uint32_t width, uint32_t height);

    bool flush();

    ID3D12Device5* getDevice() const;//device D3D12 
    uint32_t getCurrentFrame() const;
    UINT getLastCompletedFrame() const; //Deffered release

    ID3D12GraphicsCommandList* getCommandList() const;
    ID3D12CommandAllocator* getCommandAllocator() const;
    ID3D12Resource* getBackBuffer() const; //Back buffer

    D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor() const { return dsvHeap->GetCPUDescriptorHandleForHeapStart(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor() const;
    ID3D12CommandQueue* getDrawCommandQueue() const; //cola

    void SetShowGrid(bool show) { showGrid = show; }
    void SetShowAxis(bool show) { showAxis = show; }
    bool IsShowGrid() const { return showGrid; }
    bool IsShowAxis() const { return showAxis; }

private:
    bool showGrid = true;
    bool showAxis = true;

    void enableDebugLayer();
    bool createFactory();
    bool createDevice(bool useWarp);

    bool createVertexBuffer();
    bool createRootSignature();
    bool createPSO();

    DebugDrawPass* debugDraw = nullptr;
    DirectX::SimpleMath::Matrix view;
    DirectX::SimpleMath::Matrix proj;
};