#include "Globals.h"
#include "ModuleRenderer.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include "ModuleResource.h"
#include "ReadData.h"
#include "SimpleMath.h"
#include <d3d12.h>
#include "d3dx12.h"

ModuleRenderer::ModuleRenderer() : debugDraw(nullptr) {}

ModuleRenderer::~ModuleRenderer() {
    if (debugDraw) {
        delete debugDraw;
        debugDraw = nullptr;
    }
}

bool ModuleRenderer::init() {
    ModuleD3D12* d3d12 = app->getD3D12();
    if (!d3d12) return false;

    auto device = d3d12->getDevice();
    auto commandQueue = d3d12->getDrawCommandQueue();

    // Create debug draw pass
    debugDraw = new DebugDrawPass(device, commandQueue);
    if (!debugDraw) return false;

    // Create geometry
    if (!createVertexBuffer()) return false;

    // Create root signature
    if (!createRootSignature()) return false;

    // Create pipeline state
    if (!createPipelineState()) return false;

    modelLoaded = false;

    std::filesystem::path modelPath = std::filesystem::current_path() / "3rdParty" / "tinygltf" / "models" / "Duck" / "Duck.gltf";
    if (std::filesystem::exists(modelPath))
    {
        model.Load(modelPath.string().c_str());
        modelLoaded = true;
        LOG("Model loaded successfully");
    }
    else
    {
        LOG("Could not find Duck.gltf at: %s", modelPath.string().c_str());
    
    }

    return true;
}

void ModuleRenderer::preRender() {

}

void ModuleRenderer::render() {
    if (modelLoaded)
    {
       renderModel();
    }

    renderDebugDraw();
}

void ModuleRenderer::postRender() {
}

bool ModuleRenderer::createVertexBuffer() {
    struct Vertex {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector2 uv;
        DirectX::SimpleMath::Vector3 color;
    };

    // Triángulo 3D con textura
    Vertex vertices[] = {
        // Base del triángulo
        { DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f), DirectX::SimpleMath::Vector2(0.5f, 0.0f), DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f) },
        { DirectX::SimpleMath::Vector3(-1.0f, -1.0f, 0.0f), DirectX::SimpleMath::Vector2(0.0f, 1.0f), DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f) },
        { DirectX::SimpleMath::Vector3(1.0f, -1.0f, 0.0f), DirectX::SimpleMath::Vector2(1.0f, 1.0f), DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f) },

        // Cuad para textura (opcional)
        { DirectX::SimpleMath::Vector3(-1.0f, 1.0f, 0.0f), DirectX::SimpleMath::Vector2(0.0f, 0.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) },
        { DirectX::SimpleMath::Vector3(1.0f, 1.0f, 0.0f), DirectX::SimpleMath::Vector2(1.0f, 0.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) },
        { DirectX::SimpleMath::Vector3(-1.0f, -1.0f, 0.0f), DirectX::SimpleMath::Vector2(0.0f, 1.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) },
        { DirectX::SimpleMath::Vector3(1.0f, 1.0f, 0.0f), DirectX::SimpleMath::Vector2(1.0f, 0.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) },
        { DirectX::SimpleMath::Vector3(1.0f, -1.0f, 0.0f), DirectX::SimpleMath::Vector2(1.0f, 1.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) },
        { DirectX::SimpleMath::Vector3(-1.0f, -1.0f, 0.0f), DirectX::SimpleMath::Vector2(0.0f, 1.0f), DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f) }
    };

    auto resources = app->getResources();
    vertexBuffer = resources->createDefaultBuffer(vertices, sizeof(vertices), "TriangleVB");
    if (!vertexBuffer) return false;

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);
    vertexCount = sizeof(vertices) / sizeof(Vertex);

    return true;
}

bool ModuleRenderer::createRootSignature() {
    ModuleD3D12* d3d12 = app->getD3D12();
    auto device = d3d12->getDevice();

    CD3DX12_DESCRIPTOR_RANGE srvTable;
    CD3DX12_DESCRIPTOR_RANGE samplerTable;

    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); 
    samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

  
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); 

    // Material constant buffer
    rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL); 

    // Texture SRV descriptor table
    rootParameters[2].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL); 
   
    // Sampler descriptor table
    rootParameters[3].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL); 

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(4, rootParameters, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        return false;
    }

    return SUCCEEDED(device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

bool ModuleRenderer::createPipelineState() {
    ModuleD3D12* d3d12 = app->getD3D12();
    auto device = d3d12->getDevice();

    // Input layout
    D3D12_INPUT_ELEMENT_DESC layout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
       { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
       { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Leer shaders
    auto vs = DX::ReadData(L"ModelVS.cso");
    auto ps = DX::ReadData(L"ModelPS.cso");

    if (vs.empty() || ps.empty()) {
        // Fallback to simple shaders if model shaders aren't found
        LOG("Model shaders not found, trying default shaders...");

        vs = DX::ReadData(L"DefaultVS.cso");
        ps = DX::ReadData(L"DefaultPS.cso");

        if (vs.empty() || ps.empty()) {
            OutputDebugStringA("ERROR: Shaders not found!\n");
            return false;
        }
    }

    // Pipeline state description for model rendering
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { layout, _countof(layout) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { vs.data(), vs.size() };
    psoDesc.PS = { ps.data(), ps.size() };

    // Rasterizer state - IMPORTANT: Set for counter-clockwise winding (glTF default)
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;  // glTF uses CCW winding

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
}

void ModuleRenderer::renderModel()
{
    auto d3d12 = app->getD3D12();
    auto camera = app->getCamera();
    auto commandList = d3d12->getCommandList();

    if (!commandList || !pipelineState || !camera) return;

    // Configure viewport and scissor
    float w = (float)d3d12->getWindowWidth();
    float h = (float)d3d12->getWindowHeight();
    D3D12_VIEWPORT vp{ 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    D3D12_RECT sr{ 0, 0, (LONG)w, (LONG)h };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &sr);

    // Configure pipeline
    commandList->SetPipelineState(pipelineState.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    // TODO: Update root signature to include material CBV and texture

    // Pass MVP matrix
    DirectX::SimpleMath::Matrix mvp = DirectX::SimpleMath::Matrix::Identity *
        camera->getViewMatrix() *
        camera->getProjectionMatrix();
    mvp = mvp.Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, 16, &mvp, 0);

    // Render the model
    model.Render(commandList);
}

void ModuleRenderer::renderDebugDraw() {
    if (!debugDraw) return;

    auto d3d12 = app->getD3D12();
    auto camera = app->getCamera();
    auto commandList = d3d12->getCommandList();

    if (!commandList || !camera) return;

    // Dibujar grid si está activado
    if (showGrid) {
        dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    // Dibujar ejes si está activado
    if (showAxis) {
        DirectX::SimpleMath::Matrix identity = DirectX::SimpleMath::Matrix::Identity;
        dd::axisTriad(&identity._11, 0.1f, 1.5f);
    }

    // Grabar comandos de debug draw
    debugDraw->record(commandList,
        d3d12->getWindowWidth(),
        d3d12->getWindowHeight(),
        camera->getViewMatrix(),
        camera->getProjectionMatrix());
}