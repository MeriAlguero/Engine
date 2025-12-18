#include "Globals.h"
#include "Application.h"
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
    drawMainMenuBar();

    if (showConfigWindow) drawConfigurationWindow(); 
    if (showAboutWindow)
    {
        ImGui::Begin("About this Engine", &showAboutWindow);

        ImGui::Text("DirectX 12 Engine - Assigment 1");
        ImGui::Separator();
        ImGui::Text("Developed by: Meritxell Alguero");
        ImGui::Text("This engine is still in development...");

        if (ImGui::Button("Close"))
        {
            showAboutWindow = false; 
        }

        ImGui::End();
    }

    auto cmdList = app->getD3D12()->getCommandList();
    imguiPass->record(cmdList);
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
                app->requestQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Configuration", NULL, showConfigWindow))
            {
                showConfigWindow = !showConfigWindow;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About")) { showAboutWindow = true; }
            if (ImGui::MenuItem("ImGui Demo")) { ShellExecute(NULL, L"open", L"https://github.com/ocornut/imgui", NULL, NULL, SW_SHOWNORMAL); } 
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        app->requestQuit();
    }
}

void ModuleImGui::drawConfigurationWindow()
{
    if (ImGui::Begin("Configuration", &showConfigWindow))
    {
        ModuleD3D12* d3d12 = app->getD3D12();

        float fps = ImGui::GetIO().Framerate;
        float ms = 1000.0f / fps;
        static int offset = 0;
        fpsLog[offset] = fps;
        offset = (offset + 1) % IM_ARRAYSIZE(fpsLog);

        ImGui::Text("Application");
        ImGui::PlotHistogram("##framerate", fpsLog, IM_ARRAYSIZE(fpsLog), 0, "Framerate", 0.0f, 100.0f, ImVec2(0, 80));


        ImGui::Text("Framerate %.2f", fps);
        ImGui::Text("Milliseconds %.2f", ms);

        ImGui::Separator();
        ImGui::Text("Debug Gizmos");
        bool grid = d3d12->IsShowGrid();
        bool axis = d3d12->IsShowAxis();
        if (ImGui::Checkbox("Show Grid", &grid)) {
            d3d12->SetShowGrid(grid);
        }
        if (ImGui::Checkbox("Show Axis", &axis)) {
            d3d12->SetShowAxis(axis);
        }

        ImGui::Separator();
        ImGui::Text("Texture Filtering");

        const char* items[] = { "Wrap / Bilinear", "Clamp / Bilinear", "Wrap / Point", "Clamp / Point" };
        static int item_current = 0;
        if (ImGui::Combo("Sampler Mode", &item_current, items, IM_ARRAYSIZE(items)))
        {
            //app->getD3D12()->setSamplerMode(item_current);
        }
    }
    ImGui::End();
}