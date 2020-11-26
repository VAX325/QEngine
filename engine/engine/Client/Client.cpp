#include <Base_include.h>
#include "Client.h"
#include "CUIManager.h"
#include "../xmlparser.h"
#include "../Utils.h"

#include <iostream>
#include <ctime>

HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;

int g_iWindowWidth = 800;
int g_iWindowHeight = 600;

bool g_bApplicationState = true;

HFONT hFont;

IDirect3D9* g_pDirect3D = NULL;
IDirect3DDevice9* g_pDirect3DDevice = NULL;

#include "Input.h"

DxInput InputObj;

CLogManager LogManager;

FileSystem fs;

CSpriteManager CSM;

CSoundManager SoundManager;

CScriptSystem ScriptSystem;

CUIManager UIManager;

XMLParser* xml_parser;

lua_State* L;

bool Sound = false;

float fps = 0.0f;
static DWORD total = 0;
static DWORD frames = 0;

float LuaGetFPS()
{
	return fps;
}

int ClientMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
#ifdef _DEBUG
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
	freopen("CONIN$", "w+", stdin);
#endif // _DEBUG

	fs = FileSystem();

	LogManager.Init();

	InputObj = DxInput();

	if (FindWord((char*)lpCmdLine, (char*)"-nosound"))
	{
		Sound = true;
		LogManager.LogMsg((char*)"No sound!");
	}

	g_hWnd = NULL;
	g_hInstance = GetModuleHandle(NULL);

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = "WindowClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		LogManager.LogError((char*)"Can't register window class", true);
	}

	LogManager.LogMsg((char*)"Register window class");

	g_hWnd = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		"WindowClass",
		"Master of Dungeon",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT, CW_USEDEFAULT,
		g_iWindowWidth,
		g_iWindowHeight,
		NULL,
		NULL,
		g_hInstance,
		NULL);

	if (g_hWnd == NULL)
	{
		LogManager.LogError((char*)"Can't create window", true);
	}

	LogManager.LogMsg((char*)"Window created");

	if (!InitDirect3D(D3DFMT_R5G6B5, D3DFMT_D16))
	{
		LogManager.LogError((char*)"Can't get DirectX context", true);
	}

	LogManager.LogMsg((char*)"Getted DirectX context");

	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);
	SetFocus(g_hWnd);
	SetForegroundWindow(g_hWnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	if (!InputObj.Initialize(g_hInstance, g_hWnd))
	{
		LogManager.LogError((char*)"Can't get DirectInput context", true);
	}

	LogManager.LogMsg((char*)"Getted DirectInput context");

	CSM = CSpriteManager();

	CSM.LoadAllSprites();

	UIManager = CUIManager();

	UIManager.LoadPanels();

	for(int i = 0; i != UIManager.GetCountOfPanels(); i++)
	{
		if(strcmp(UIManager.GetPanelName(i), "MainMenu.xml") == 0)
		{
			UIManager.ShowPanel(i);
			break;
		}
	}

	if (!Sound)
	{
		SoundManager = CSoundManager();

		SoundManager.PlayBuffSound(0, 1.0f, true, 0.5, 1.0);
	}

	L = luaL_newstate();
	luaL_openlibs(L);

	ScriptSystem = CScriptSystem();

	InitLuaShared(L, &LogManager);

	DWORD start = GetTickCount();

	DrawFrame();

	DWORD end = GetTickCount();

	total += end - start;

	frames++;

	if (total >= 1000) {
		fps = frames * 1000 / total;

#ifdef _DEBUG
		cout << "FPS: " << fps << "\n";
#endif
		frames = 0;
		total = 0;
	}

	ScriptSystem.LuaStart(L);
	
	while (g_bApplicationState)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DWORD start = GetTickCount();

			DrawFrame();

			DWORD end = GetTickCount();

			total += end - start;

			frames++;

			if (total >= 1000) {
				fps = frames * 1000 / total;

#ifdef _DEBUG
				//cout << "FPS: " << fps << "\n";
#endif
				frames = 0;
				total = 0;
			}
		}

		ScriptSystem.LuaUpdate(L);

		if(!g_bApplicationState)
		{
			Shutdown();
		}
	}

	Shutdown();
	return 0;
}

long WINAPI WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_DESTROY:
	{
		g_bApplicationState = false;
		return 0;
	}
	}

	return DefWindowProc(hWnd, iMsg, wParam, lParam);  //���� ���� ��� ��� ������ ���������, ����� ��� ������������ �������
}

bool InitDirect3D(D3DFORMAT ColorFormat, D3DFORMAT DepthFormat)
{
	if ((g_pDirect3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	D3DPRESENT_PARAMETERS PresParam;
	ZeroMemory(&PresParam, sizeof(PresParam));

	HRESULT hr = NULL;

	D3DDISPLAYMODE DisplayMode;
	hr = g_pDirect3D->GetAdapterDisplayMode(
		D3DADAPTER_DEFAULT,
		&DisplayMode);

	if (FAILED(hr))
		return false;

	PresParam.hDeviceWindow = g_hWnd;
	PresParam.Windowed = true;
	PresParam.BackBufferWidth = g_iWindowWidth;
	PresParam.BackBufferHeight = g_iWindowHeight;
	PresParam.BackBufferCount = 1;
	PresParam.EnableAutoDepthStencil = true;
	PresParam.AutoDepthStencilFormat = DepthFormat;
	PresParam.SwapEffect = D3DSWAPEFFECT_FLIP;
	PresParam.BackBufferFormat = DisplayMode.Format;

	hr = g_pDirect3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		g_hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&PresParam,
		&g_pDirect3DDevice);

	if (SUCCEEDED(hr))
		return true;

	hr = g_pDirect3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		g_hWnd,
		D3DCREATE_MIXED_VERTEXPROCESSING,
		&PresParam,
		&g_pDirect3DDevice);

	if (SUCCEEDED(hr))
		return true;

	hr = g_pDirect3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		g_hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&PresParam,
		&g_pDirect3DDevice);

	if (SUCCEEDED(hr))
		return true;

	return false;
}

void DrawFrame()
{
	HRESULT hr = g_pDirect3DDevice->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST)
		return;

	g_pDirect3DDevice->Clear(
		0L,
		NULL,
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB(0, 0, 0),
		1.0f,
		0L);

	if (InputObj.Frame())
	{
		if (InputObj.IsKeyPressed(DIK_ESCAPE, 0x80))
		{
			g_bApplicationState = false;
		}
		if (InputObj.IsLMouseButtonPressed())
		{
			
		}
		if (InputObj.IsRMouseButtonPressed())
		{
			
		}
	}
	else
	{
		g_bApplicationState = false;
	}

	g_pDirect3DDevice->BeginScene();

	CSM.RenderAllSprites();
	
	if(!Sound)
	{
		SoundManager.Update();
	}
	
	//D3DXCreateFont(g_pDirect3DDevice, 30, 10, 1, 0, FALSE, 0, 30, 0, 0, "Arial", &pFont);
	//DrawTextD3D(g_pDirect3DDevice, pFont, FPSC, 0, 0, 1000, 700, D3DCOLOR_ARGB(250, 100, 100, 100));

	UIManager.RenderPanels();

	g_pDirect3DDevice->EndScene();

	g_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
}

void Shutdown()
{
	if (g_pDirect3DDevice != NULL)
	{
		g_pDirect3DDevice->Release();
		g_pDirect3DDevice = NULL;
	}

	if (g_pDirect3D != NULL)
	{
		g_pDirect3D->Release();
		g_pDirect3D = NULL;
	}

	InputObj.ShutdownInput();

	if (!DestroyWindow(g_hWnd))
		g_hWnd = NULL;

	if (!UnregisterClass("WindowClass", g_hInstance))
		g_hInstance = NULL;
}

IDirect3DDevice9* GetD3D9Device()
{
	return g_pDirect3DDevice;
}

HWND GetMainWnd()
{
	return g_hWnd;
}

int GetWindowH()
{
	return g_iWindowHeight;
}

int GetWindowW()
{
	return g_iWindowWidth;
}

CLogManager* GetLogObjCl()
{
	return &LogManager;
}

CSpriteManager* GetSpriteManger()
{
	return &CSM;
}

CSoundManager* GetSoundObj()
{
	return &SoundManager;
}

FileSystem* GetFileSystemObjCl() 
{
	return &fs;
}

CScriptSystem* GetScriptSystemObjCl()
{
	return &ScriptSystem;
}

lua_State* GetLuaStateCl()
{
	return L;
}