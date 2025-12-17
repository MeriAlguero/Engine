#include "Globals.h"
#include "Application.h"
#include "ModuleImGui.h"
#include "ModuleD3D12.h"
#include "ModuleImGui.h"

#include "imgui.h"
#include "3rdParty/imgui-docking/backends/imgui_impl_win32.h"
#include "3rdParty/imgui-docking/backends/imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_5.h>
#include <tchar.h>
#include <Windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WinProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ModuleImGui::ModuleImGui()
{
}

ModuleImGui::~ModuleImGui()
{
}

bool ModuleImGui::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    HWND hWnd = static_cast<HWND>(app->getWindow());

    // Inicializamos tu clase pasando el dispositivo y la ventana
    imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), hWnd);

    return true;
}
void ModuleImGui::preRender()
{
    imguiPass->startFrame();
}

void ModuleImGui::render()
{
    auto d3d12 = app->getD3D12();
    auto cmdList = d3d12->getCommandList();
    auto cmdAlloc = d3d12->getCommandAllocator();

    cmdList->Reset(cmdAlloc, nullptr);

    drawMainMenuBar();
    drawConfigurationWindow();
    if (ImGui::Begin("Editor")) {
        ImGui::Text("Hello from ImGui!");
    }
    ImGui::End();

    ImGui::Render();

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        d3d12->getBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
   
    imguiPass->record(cmdList);
    
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        d3d12->getBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->Close();
    ID3D12CommandList* lists[] = { cmdList };
    d3d12->getDrawCommandQueue()->ExecuteCommandLists(1, lists);
}

void ModuleImGui::postRender()
{

    ModuleD3D12* d3d12 = app->getD3D12();

   // imguiPass->record(d3d12->getCommandList());

}
void ModuleImGui::drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "ESC"))
            {
                // Application::getInstance()->requestQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Configuration", NULL, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About")) {}
            if (ImGui::MenuItem("ImGui Demo")) {} // drawDemoWindow()
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ModuleImGui::drawConfigurationWindow()
{
    if (ImGui::Begin("Configuration"))
    {
        ImGui::Text("Application");
        ImGui::PlotHistogram("##framerate", fpsLog, IM_ARRAYSIZE(fpsLog), 0, "Framerate", 0.0f, 100.0f, ImVec2(0, 80));

        float lastFPS = 0.0f;
        float lastMs = 0.0f; 

        ImGui::Text("Framerate %.2f", lastFPS);
        ImGui::Text("Milliseconds %.2f", lastMs);

        ImGui::Separator();
        ImGui::Text("Window");
       
        const char* items[] = { "Wrap / Bilinear", "Clamp / Bilinear", "Wrap / Point", "Clamp / Point" };
        static int item_current = 0;
        if (ImGui::Combo("Sampler Mode", &item_current, items, IM_ARRAYSIZE(items)))
        {
            // Aquí avisarás a tu ModuleD3D12 para que cambie el Sampler
        }
    }
    ImGui::End();
}