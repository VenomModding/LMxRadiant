#include "STDInclude.hpp"

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

CMainFrame* CMainFrame::ActiveWindow;

// Taken directly from q3radiant
// https://github.com/id-Software/Quake-III-Arena
#define W_CAMERA		0x0001
#define W_XY			0x0002
#define W_XY_OVERLAY	0x0004
#define W_Z				0x0008
#define W_TEXTURE		0x0010
#define W_Z_OVERLAY		0x0020
#define W_CONSOLE		0x0040
#define W_ENTITY		0x0080
#define W_CAMERA_IFON	0x0100
#define W_XZ			0x0200  //--| only used for patch vertex manip stuff
#define W_YZ			0x0400  //--|
#define W_GROUP			0x0800 
#define W_MEDIA			0x1000 
#define W_ALL			0xFFFFFFFF

void __declspec(naked) CMainFrame::hk_RoutineProcessing(void)
{
	__asm
	{
		push ecx
		mov ecx, eax // eax = this :: ecx => this(__stdcall)
		
		mov CMainFrame::ActiveWindow, ecx;
		
		call CMainFrame::RoutineProcessing
		pop ecx
		retn
	}
}

void CMainFrame::RoutineProcessing()
{
	//this->m_wndStatusBar.vtbl().SetStatusText(&this->m_wndStatusBar, 0x75);
	
	if (!this->m_bDoLoop)
	{
		return;
	}

	if (0.0 == Game::g_time)
	{
		Game::g_time = 0.0;
	}

	if (0.0 == Game::g_oldtime)
	{
		Game::g_oldtime = 0.0;
	}

	const double time = clock() / 1000.0;
	double oldtime = time - Game::g_time;

	Game::g_time = time;

	if (oldtime > 2.0)
	{
		oldtime = 0.1;
	}

	Game::g_oldtime = oldtime;

	if (oldtime > 0.2)
	{
		oldtime = 0.2;
	}

	if (this->m_pCamWnd)
	{
		const auto delta = static_cast<float>(oldtime);
		this->m_pCamWnd->Cam_MouseControl(delta);
	}

	if (Game::g_nUpdateBits)
	{
		const int nBits = Game::g_nUpdateBits;
		Game::g_nUpdateBits = 0;
		this->UpdateWindows(nBits);
	}

	Game::Globals::radiant_initiated = true;
}

bool Worldspawn_OnKeyChange(const Game::epair_t* epair, const char* key, float* value, const int &valueSize)
{
	if (!Utils::Q_stricmp(epair->key, key))
	{
		bool changed = false;
		
		std::vector<std::string> KeyValues = Utils::Explode(epair->value, ' ');

		int count = KeyValues.size();
		if (count > valueSize) count = valueSize;

		for (auto i = 0; i < count; i++)
		{
			float temp = Utils::try_stof(KeyValues[i], true);

			if (value[i] != temp)
			{
				value[i] = temp;
				changed = true;
			}
		}

		if (changed)
		{
			return true;
		}
	}
	
	return false;
}

void TrackWorldspawnSettings()
{
	// trackWorldspawn
	
	if (const auto world = GET_WORLDENTITY; 
		world && world->firstActive->eclass->name)
	{
		if (!Utils::Q_stricmp(world->firstActive->eclass->name, "worldspawn"))
		{
			for (auto epair = world->firstActive->epairs; epair; epair = epair->next)
			{
				if (Worldspawn_OnKeyChange(epair, "sundirection", Game::Globals::trackWorldspawn.sundirection, 3))
				{
					if (Game::Globals::trackWorldspawn.initiated)
					{
						Components::RemNet::Cmd_SendDvar(Utils::VA("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f %.1f %.1f\"\n}", "r_lighttweaksundirection",
							Game::Globals::trackWorldspawn.sundirection[0], Game::Globals::trackWorldspawn.sundirection[1], Game::Globals::trackWorldspawn.sundirection[2]));
					}
				}

				if (Worldspawn_OnKeyChange(epair, "suncolor", Game::Globals::trackWorldspawn.suncolor, 3))
				{
					if (Game::Globals::trackWorldspawn.initiated)
					{
						Components::RemNet::Cmd_SendDvar(Utils::VA("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f %.1f %.1f\"\n}", "r_lighttweaksuncolor",
							Game::Globals::trackWorldspawn.suncolor[0], Game::Globals::trackWorldspawn.suncolor[1], Game::Globals::trackWorldspawn.suncolor[2]));
					}
				}

				if (Worldspawn_OnKeyChange(epair, "sunlight", &Game::Globals::trackWorldspawn.sunlight, 1))
				{
					if (Game::Globals::trackWorldspawn.initiated)
					{
						Components::RemNet::Cmd_SendDvar(Utils::VA("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f\"\n}", "r_lighttweaksunlight",
							Game::Globals::trackWorldspawn.sunlight));
					}
				}
			}
		}

		if (!Game::Globals::trackWorldspawn.initiated)
		{
			Game::Globals::trackWorldspawn.initiated = true;
			return;
		}
	}
}


void CMainFrame::UpdateWindows(int nBits)
{
	// grab camera if not using floating windows
	if (!Game::Globals::radiant_floatingWindows && this->m_pCamWnd)
	{
		CCamWnd::ActiveWindow = this->m_pCamWnd;
		Game::Globals::radiant_floatingWindows = true;
	}

	if (!Game::g_bScreenUpdates)
	{
		return;
	}

	if (Game::Globals::live_connected) 
	{
		TrackWorldspawnSettings();
	}

	// check d3d device or we ASSERT in R_CheckHwnd_or_Device (!dx_device) when using floating windows
	if (!Game::Globals::d3d9_device)
	{
		return;
	}
	
	if (nBits & (W_XY | W_XY_OVERLAY))
	{
		if (this->m_pXYWnd)
		{
			m_pXYWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	if (nBits & W_CAMERA || ((nBits & W_CAMERA_IFON) && this->m_bCamPreview))
	{
		if (this->m_pCamWnd)
		{
			
#if !CCAMWND_REALTIME
			// Redraw the camera view
			m_pCamWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
#endif
			
			Game::Globals::m_pCamWnd_ref = m_pCamWnd;

			// on update cam window through the 2d grid or something else
			if ((nBits & W_CAMERA_IFON) && this->m_bCamPreview || nBits < 0 || nBits == 3)
			{
			}

			// only update the remote cam when we actually move it, not when we update the cam window by doing something in the gridWnd etc.
			else if(Game::Globals::live_connected)
			{
				// Attempt to update the remote camera
				if (CCamWnd::ActiveWindow)
				{
					Components::RemNet::Cmd_SendCameraUpdate(CCamWnd::ActiveWindow->camera.origin, CCamWnd::ActiveWindow->camera.angles);
				}
			}
		}
	}

	if (nBits & (W_Z | W_Z_OVERLAY))
	{
		if (this->m_pZWnd)
		{
			m_pZWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	if (nBits & W_TEXTURE)
	{
		if (this->m_pTexWnd) 
		{
			m_pTexWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
}

// typedef CFrameWnd::DefWindowProc
typedef LRESULT(__thiscall* wndproc_t)(CMainFrame*, UINT Msg, WPARAM wParam, LPARAM lParam);
/* ------------------------- */ wndproc_t o_wndproc = reinterpret_cast<wndproc_t>(0x584D97);

// hook windowproc to always update the camera window on mouse movement + imgui io (not including mouse clicks)
LRESULT __fastcall CMainFrame::WindowProc(CMainFrame* pThis, [[maybe_unused]] void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// update the window on mouse movement (everywhere within windows)
	// * having sunlight preview enabled causes a crash upon loading a new map => 0x4067CD => see 'sunlight_preview_arg_check'
	/*if(pThis->m_pCamWnd)
	{
		pThis->UpdateWindows(W_CAMERA);
	}*/

	if (GGUI_READY)
	{
		// handle mouse cursor for open menus
		for (auto menu = 0; menu < GGUI_MENU_COUNT; menu++)
		{
			if (Game::Globals::gui.menus[menu].menustate)
			{
				if (ImGui_ImplWin32_WndProcHandler(pThis->GetWindow(), Msg, wParam, lParam))
				{
					//if (ImGui::GetIO().WantCaptureMouse)
					//{
					//	ShowCursor(0);
					//	ImGui::GetIO().MouseDrawCursor = 1;
					//	return true;
					//}
				}
			}
		}

		//ShowCursor(1);
		//ImGui::GetIO().MouseDrawCursor = 0;
	}

	// => CFrameWnd::DefWindowProc
	return o_wndproc(pThis, Msg, wParam, lParam);
}


// *
// | -------------------- MSG typedefs ------------------------
// *


typedef void(__thiscall* on_cmainframe_scroll)(CMainFrame*, UINT, SHORT, CPoint);
	on_cmainframe_scroll __on_mscroll;


typedef void(__thiscall* on_cmainframe_key)(CMainFrame*, UINT, UINT, UINT);
	on_cmainframe_key __on_keydown;

typedef void(__stdcall* on_cmainframe_keyup)(CMainFrame*, UINT);
	on_cmainframe_keyup __on_keyup;


// *
// | -------------------- Mouse Scroll ------------------------
// *

void __fastcall CMainFrame::on_mscroll(CMainFrame* pThis, [[maybe_unused]] void* edx, UINT nFlags, SHORT zDelta, CPoint point)
{
	Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_MOUSEWHEEL, zDelta);

#if !CCAMWND_REALTIME
	pThis->UpdateWindows(W_CAMERA);
#endif
	
	// do not pass msg if mouse is inside an imgui window
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		return __on_mscroll(pThis, nFlags, zDelta, point);
	}
}

// *
// | ------------------------ Key ----------------------------
// *

void __fastcall CMainFrame::on_keydown(CMainFrame* pThis, [[maybe_unused]] void* edx, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_KEYDOWN, 0, nChar);

#if !CCAMWND_REALTIME
	pThis->UpdateWindows(W_CAMERA);
#endif
	
	// do not pass the msg if mouse is inside an imgui window
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		return __on_keydown(pThis, nChar, nRepCnt, nFlags);
	}
}

void __stdcall CMainFrame::on_keyup(CMainFrame* pThis, UINT nChar)
{
	Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_KEYUP, 0, nChar);

#if !CCAMWND_REALTIME
	pThis->UpdateWindows(W_CAMERA);
#endif
	
	// do not pass the msg if mouse is inside an imgui window
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		return __on_keyup(pThis, nChar);
	}
}

// *
// | ----------------------------------------------------------
// *


// check for nullptr (world_entity)
void __declspec(naked) sunlight_preview_arg_check()
{
	const static uint32_t retn_pt = 0x4067D0;
	const static uint32_t onzero_retn_pt = 0x4067E0;
	__asm
	{
		mov		[ebp - 20DCh], ecx; // og
		
		pushad;
		test	edx, edx;			// world_entity
		jz		ENT_IS_ZERO;

		popad;
		mov     esi, [edx + 8];		// og
		jmp		retn_pt;

		ENT_IS_ZERO:
		popad;
		jmp		onzero_retn_pt;
	}
}

void CMainFrame::main()
{
	// hook MainFrameWnd continuous thread
	Utils::Hook(0x421A90, CMainFrame::hk_RoutineProcessing, HOOK_JUMP).install()->quick();

	// this might be needed later, not useful for the camera window tho
	// hook windowproc to always update the camera window on mouse movement + imgui io (not including mouse clicks)
	//Utils::Hook(0x421A7B, CMainFrame::WindowProc, HOOK_CALL).install()->quick();

	
	// *
	// detour cmainframe member functions to get imgui input
	
	__on_mscroll	= reinterpret_cast<on_cmainframe_scroll>(Utils::Hook::Detour(0x42B850, on_mscroll, HK_JUMP));
	__on_keydown	= reinterpret_cast<on_cmainframe_key>(Utils::Hook::Detour(0x422370, on_keydown, HK_JUMP));
	__on_keyup		= reinterpret_cast<on_cmainframe_keyup>(Utils::Hook::Detour(0x422270, on_keyup, HK_JUMP));

	// check for nullptr (world_entity) in a sunlight preview function. Only required with the ^ hook, see note there.
	Utils::Hook::Nop(0x4067C7, 6);
	Utils::Hook(0x4067C7, sunlight_preview_arg_check, HOOK_JUMP).install()->quick();
}
