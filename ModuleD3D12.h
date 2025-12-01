#pragma once
#include "Globals.h"
#include "Application.h"
#include "Module.h"
#include <dxgi1_6.h>

class ModuleD3D12 : public Module
{
public:

   ModuleD3D12(HWND hWnd);
   void enableDebugLayer();
   void createDevice();

};
