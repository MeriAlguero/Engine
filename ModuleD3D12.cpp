#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleImGui.h"
#include "ModuleCamera.h"
#include "ReadData.h"
#include "SimpleMath.h"
#include <d3d12.h>
#include "d3dx12.h"

ModuleD3D12::ModuleD3D12(HWND wnd) : hWnd(wnd) {}

ModuleD3D12::~ModuleD3D12() {
    
    // Limpiar fence event
    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }
}

bool ModuleD3D12::init()
{
#if defined(_DEBUG)
    enableDebugLayer();
#endif 

    bool ok = createFactory();
    ok = ok && createDevice(false);

    if (ok)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ok = SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

        if (ok)
            ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

        if (ok)
        {
            ok = SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
        }
    }

    if (ok)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = BACK_BUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ok = SUCCEEDED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        if (ok) rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);
    this->windowWidth = (clientRect.right - clientRect.left > 0) ? clientRect.right - clientRect.left : 1280;
    this->windowHeight = (clientRect.bottom - clientRect.top > 0) ? clientRect.bottom - clientRect.top : 720;

    if (ok)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = windowWidth;
        swapChainDesc.Height = windowHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ComPtr<IDXGISwapChain1> swapChain1;
        ok = SUCCEEDED(factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
        if (ok) ok = SUCCEEDED(swapChain1.As(&swapChain));
        if (ok) currentFrame = swapChain->GetCurrentBackBufferIndex();
    }

    if (ok)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < BACK_BUFFER_COUNT; ++i)
        {
            ok = SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            if (!ok) break;
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(rtvDescriptorSize);
        }
    }

    if (ok)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ok = SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

        if (ok)
        {
            dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            D3D12_CLEAR_VALUE depthClear = { DXGI_FORMAT_D32_FLOAT, {1.0f, 0} };
            auto hProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto rDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            ok = SUCCEEDED(device->CreateCommittedResource(&hProps, D3D12_HEAP_FLAG_NONE, &rDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&depthStencilBuffer)));
            if (ok) device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
        }
    }

    if (ok) ok = SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    if (ok) fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (ok)
    {
        // Solo inicializar command list básico, sin crear geometría
        commandAllocator->Reset();
        commandList->Reset(commandAllocator.Get(), nullptr);
        commandList->Close();

        ID3D12CommandList* lists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, lists);

        flush();
    }

    return ok; 
}



void ModuleD3D12::preRender() {
    flush();

    currentFrame = swapChain->GetCurrentBackBufferIndex();

    commandAllocator->Reset();
    // Cambiar nullptr en lugar de pso.Get() ya que pso ya no existe aquí
    commandList->Reset(commandAllocator.Get(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void ModuleD3D12::postRender() {
    if (!commandList || !commandQueue) return;

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);
    fenceValue++;
    commandQueue->Signal(fence.Get(), fenceValue);
}

bool ModuleD3D12::flush() {
    fenceValue++;
    commandQueue->Signal(fence.Get(), fenceValue);
    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    currentFrame = swapChain->GetCurrentBackBufferIndex();
    return true;
}

void ModuleD3D12::enableDebugLayer() {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) debug->EnableDebugLayer();
}

bool ModuleD3D12::createFactory() {
    UINT flags = 0;
#if defined(_DEBUG)
    flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    return SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));
}

bool ModuleD3D12::createDevice(bool useWarp) {
    if (useWarp) {
        ComPtr<IDXGIAdapter1> adp;
        factory->EnumWarpAdapter(IID_PPV_ARGS(&adp));
        return SUCCEEDED(D3D12CreateDevice(adp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
    }

    ComPtr<IDXGIAdapter4> adp;
    factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adp));

    return SUCCEEDED(D3D12CreateDevice(adp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
}

void ModuleD3D12::resize(uint32_t width, uint32_t height) {
    if (!device || !swapChain) return;
    windowWidth = width; windowHeight = height;

    if (app->getCamera()) {
        app->getCamera()->SetAspectRatio((float)width / (float)height);
    }

    flush();

    for (int i = 0; i < BACK_BUFFER_COUNT; ++i) renderTargets[i].Reset();

    swapChain->ResizeBuffers(BACK_BUFFER_COUNT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    currentFrame = swapChain->GetCurrentBackBufferIndex();
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < BACK_BUFFER_COUNT; ++i) {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvH);
        rtvH.Offset(rtvDescriptorSize);
    }
    depthStencilBuffer.Reset();

    D3D12_CLEAR_VALUE dClear = { DXGI_FORMAT_D32_FLOAT, {1.0f, 0} };
    auto hP = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto rD = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    device->CreateCommittedResource(&hP, D3D12_HEAP_FLAG_NONE, &rD, D3D12_RESOURCE_STATE_DEPTH_WRITE, &dClear, IID_PPV_ARGS(&depthStencilBuffer));
    device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

ID3D12Device5* ModuleD3D12::getDevice() const { return device.Get(); }
uint32_t ModuleD3D12::getCurrentFrame() const { return currentFrame; }
UINT ModuleD3D12::getLastCompletedFrame() const { return (UINT)fence->GetCompletedValue(); }
ID3D12GraphicsCommandList* ModuleD3D12::getCommandList() const { return commandList.Get(); }
ID3D12CommandAllocator* ModuleD3D12::getCommandAllocator() const { return commandAllocator.Get(); }
ID3D12CommandQueue* ModuleD3D12::getDrawCommandQueue() const { return commandQueue.Get(); }
ID3D12Resource* ModuleD3D12::getBackBuffer() const {
    return renderTargets[currentFrame].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getRenderTargetDescriptor() const {
    CD3DX12_CPU_DESCRIPTOR_HANDLE h(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    h.Offset(currentFrame, rtvDescriptorSize);
    return h;
}