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
    if (debugDraw) {
        delete debugDraw;
        debugDraw = nullptr;
    }
}

bool ModuleD3D12::init()
{
#if defined(_DEBUG)
    enableDebugLayer();
#endif 

    bool ok = createFactory();
    ok = ok && createDevice(false);
    bool ok_triangle = false;

    if (ok)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ok = SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

        if (ok) {
            debugDraw = new DebugDrawPass(device.Get(), commandQueue.Get());
        }

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
        commandAllocator->Reset();
        commandList->Reset(commandAllocator.Get(), nullptr);

        ok_triangle = createVertexBuffer();
        ok_triangle = ok_triangle && createRootSignature();
        ok_triangle = ok_triangle && createPSO();

        if (ok_triangle)
        {
           /* textureDog = app->getResources()->createTextureFromFile(L"../Game/Assets/Textures/dog.dds");

                if (textureDog)
                {
                    dogDescriptor = app->getShaderDescriptors()->allocTable(1);
                    dogDescriptor.createTextureSRV(textureDog.Get());
                }*/
        }

        commandList->Close();
        ID3D12CommandList* lists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, lists);

        flush();
    }

    return ok && ok_triangle;
}

void ModuleD3D12::render()
{
    using namespace DirectX::SimpleMath;

    if (!commandList || !commandAllocator || !pso || !debugDraw) return;

    float w = (float)windowWidth;
    float h = (float)windowHeight;
    D3D12_VIEWPORT vp{ 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    D3D12_RECT sr{ 0, 0, (LONG)w, (LONG)h };

    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &sr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    auto camera = app->getCamera();
    view = camera->getViewMatrix();
    proj = camera->getProjectionMatrix();

    Matrix mvp = (Matrix::Identity * view * proj).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvp, 0);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->DrawInstanced(vertexCount, 1, 0, 0);

    /*
    auto shaderDescriptors = app->getShaderDescriptors();
    if (shaderDescriptors != nullptr && textureDog != nullptr) {
        ID3D12DescriptorHeap* heap = shaderDescriptors->getHeap();
        ID3D12DescriptorHeap* heaps[] = { heap };
        commandList->SetDescriptorHeaps(1, heaps);
        commandList->SetGraphicsRootDescriptorTable(1, dogDescriptor.getGPUHandle());
    }*/


    dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    
    Matrix id = Matrix::Identity;
    dd::axisTriad(&id._11, 0.1f, 1.5f); 

    Vector3 origin = { 0,0,0 };
    Vector3 dest = { 1,0,0 };
    dd::line(&origin.x, &dest.x, dd::colors::Red);

    debugDraw->record(commandList.Get(), (uint32_t)windowWidth, (uint32_t)windowHeight, view, proj);

    CD3DX12_RESOURCE_BARRIER barrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrierPresent);
}

void ModuleD3D12::preRender() {
    flush();
    currentFrame = swapChain->GetCurrentBackBufferIndex();
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), pso.Get());
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

struct Vertex {
    float x, y, z;
    float u, v;
};

bool ModuleD3D12::createVertexBuffer() {
    
    static Vertex vertices[6] = {

        { -0.5f,  0.5f, 0.0f,  0.0f, 0.0f }, // Top-left
        {  0.5f, -0.5f, 0.0f,  1.0f, 1.0f }, // Bottom-right
        { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f }, // Bottom-left

        { -0.5f,  0.5f, 0.0f,  0.0f, 0.0f }, // Top-left
        {  0.5f,  0.5f, 0.0f,  1.0f, 0.0f }, // Top-right
        {  0.5f, -0.5f, 0.0f,  1.0f, 1.0f }  // Bottom-right
    };

    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "QuadVB");
    vertexCount = 6;
   
    if (!vertexBuffer) return false;
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);
    
    return true;
}

bool ModuleD3D12::createRootSignature() {
   
    CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); 


    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(2, rootParameters, 1, &linearSampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) OutputDebugStringA((char*)error->GetBufferPointer());
        return false;
    }

    return SUCCEEDED(device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

bool ModuleD3D12::createPSO() {
    D3D12_INPUT_ELEMENT_DESC layout[] = {
     { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
     { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    auto vs = DX::ReadData(L"SamplerTestVS.cso");
    auto ps = DX::ReadData(L"SamplerTestPS.cso");

    if (vs.empty() || ps.empty()) {
        OutputDebugStringA("ERROR: Shaders not found!\n");
        return false;
    }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.InputLayout = { layout, _countof(layout) }; 
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { vs.data(), vs.size() };
    psoDesc.PS = { ps.data(), ps.size() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; 
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
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