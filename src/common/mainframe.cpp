#include "std_include.hpp"

#define HIDE_STATUSBAR
//#define DEBUG_KEYS

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
cmainframe* cmainframe::activewnd;

#ifdef HIDE_STATUSBAR
	const static int WNDSTATUSBAR_HEIGHT = 0; // bottom statusbar height
#else
	const static int WNDSTATUSBAR_HEIGHT = 20; // bottom statusbar height
#endif

// set pos/size once on startup (if split view with detatched or attached child windows) 
bool CZWND_POS_ONCE_ON_STARTUP = false;

void cmainframe::routine_processing()
{
	if (!this->m_bDoLoop)
	{
		return;
	}

	if (0.0 == game::g_time)
	{
		game::g_time = 0.0;
	}

	if (0.0 == game::g_oldtime)
	{
		game::g_oldtime = 0.0;
	}

	const double time = clock() / 1000.0;
	double oldtime = time - game::g_time;

	game::g_time = time;

	if (oldtime > 2.0)
	{
		oldtime = 0.1;
	}

	game::g_oldtime = oldtime;

	if (oldtime > 0.2)
	{
		oldtime = 0.2;
	}

	if (this->m_pCamWnd)
	{
		const auto delta = static_cast<float>(oldtime);
		this->m_pCamWnd->mouse_control(delta); // this is used for the q3 camera mode (0)
	}

	if (game::g_nUpdateBits)
	{
		const int nBits = game::g_nUpdateBits;
		game::g_nUpdateBits = 0;
		this->update_windows(nBits);
	}

	game::glob::radiant_initiated = true;

	// toggle toolbar by dvar
	if (cmainframe::activewnd->m_wndToolBar.m_hWnd)
	{
		typedef void(__thiscall* CFrameWnd_ShowControlBar_t)(CFrameWnd*, CControlBar*, BOOL bShow, BOOL bDelay);
					  const auto CFrameWnd_ShowControlBar = reinterpret_cast<CFrameWnd_ShowControlBar_t>(0x59E9DD);
		
		bool is_visible = reinterpret_cast<CToolBar_vtbl*>(cmainframe::activewnd->m_wndToolBar.__vftable)->IsVisible(&cmainframe::activewnd->m_wndToolBar);

		if (dvars::mainframe_show_toolbar->current.enabled && !is_visible)
		{
			CFrameWnd_ShowControlBar(cmainframe::activewnd, &cmainframe::activewnd->m_wndToolBar, 1, 1);
		}
		else if(!dvars::mainframe_show_toolbar->current.enabled && is_visible)
		{
			CFrameWnd_ShowControlBar(cmainframe::activewnd, &cmainframe::activewnd->m_wndToolBar, 0, 1);
		}

		if(cmainframe::activewnd->m_wndStatusBar.m_hWnd)
		{
			//CFrameWnd_ShowControlBar(cmainframe::activewnd, &cmainframe::activewnd->m_wndStatusBar, 0, 1);

			//auto vtable = reinterpret_cast<CFrameWnd_vtbl*>(cmainframe::activewnd->__vftable);
			//vtable->RecalcLayout(cmainframe::activewnd, 1);

			//auto prefs = game::g_PrefsDlg();
			//prefs->m_nStatusSize = 20;
		}
			
	}

	// toggle menubar by dvar
	if (dvars::mainframe_show_menubar->current.enabled && !ggui::mainframe_menubar_enabled)
	{
		components::command::execute("menubar_show");
		game::CPrefsDlg_SavePrefs();
	}
	else if(!dvars::mainframe_show_menubar->current.enabled && ggui::mainframe_menubar_enabled)
	{
		components::command::execute("menubar_hide");
		game::CPrefsDlg_SavePrefs();
	}
}

void __declspec(naked) cmainframe::hk_routine_processing(void)
{
	__asm
	{
		push	ecx;
		mov		ecx, eax; // eax = this :: ecx => this(__stdcall)

		mov		cmainframe::activewnd, ecx; // update activewnd

		call	cmainframe::routine_processing;
		pop		ecx;
		retn;
	}
}

void on_createclient()
{
	radiantapp::on_create_client();

	const auto prefs = game::g_PrefsDlg();
	if (!prefs->m_nView || prefs->m_nView == 3)
	{
		if(!dvars::mainframe_show_console->current.enabled)
		{
			// disable the console pane
			reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteRow(&cmainframe::activewnd->m_wndSplit, 1);
		}
		
		if(!dvars::mainframe_show_zview->current.enabled)
		{
			if (!prefs->m_nView)
			{
				reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteColumn(&cmainframe::activewnd->m_wndSplit2, 0);
			}
			else
			{
				// if left <-> right switched (second mode / nView 3)
				reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteColumn(&cmainframe::activewnd->m_wndSplit2, 2);
			}
		}
	}

	// ensure prefs are up to date
	if (dvars::gui_use_new_context_menu)
	{
		prefs->m_bRightClick = !dvars::gui_use_new_context_menu->current.enabled;
	}

	// hide original windows and show the z-view (rendering canvas for imgui)
	if (cmainframe::activewnd)
	{
		if (cmainframe::activewnd->m_pXYWnd)
		{
			ShowWindow(cmainframe::activewnd->m_pXYWnd->GetWindow(), SW_HIDE);
		}

		if (cmainframe::activewnd->m_pCamWnd)
		{
			ShowWindow(cmainframe::activewnd->m_pCamWnd->GetWindow(), SW_HIDE);
		}

		if (cmainframe::activewnd->m_pZWnd)
		{
			ShowWindow(cmainframe::activewnd->m_pZWnd->GetWindow(), SW_SHOW);
		}

		if (cmainframe::activewnd->m_pTexWnd)
		{
			ShowWindow(cmainframe::activewnd->m_pTexWnd->GetWindow(), SW_HIDE);
		}

		if (cmainframe::activewnd->m_pFilterWnd)
		{
			ShowWindow(cmainframe::activewnd->m_pFilterWnd->GetWindow(), SW_HIDE);
		}

		ShowWindow(cmainframe::activewnd->m_wndStatusBar.m_hWnd, SW_HIDE);

		//ShowWindow(components::renderer::get_window(components::renderer::CFXWND)->hwnd, SW_SHOW);
	}
}

void __declspec(naked) hk_on_createclient()
{
	const static uint32_t retn_addr = 0x4232F3;
	__asm
	{
		pushad;
		call	on_createclient;
		popad;

		mov     eax, 1; // og
		jmp		retn_addr;
	}
}

// auto load t5xradiant.prj
void create_qe_children(cmainframe* mainframe)
{
	char app_path[_MAX_PATH + 1];
	GetModuleFileNameA(nullptr, app_path, _MAX_PATH);

	std::string project_path = app_path;
	utils::replace(project_path, ".exe", ".prj");

	const bool bProjectLoaded = utils::hook::call<bool(__fastcall)(const char*)>(0x48BAB0)(project_path.c_str());

	if(!bProjectLoaded)
	{
		game::Com_Error("Unable to load project file <t5xradiant.prj>");
	}

	auto* m_camera_origin = reinterpret_cast<float*>(0x241A5A4);
	float& z_scale = *reinterpret_cast<float*>(0x241A5B0);

	m_camera_origin[0] = 0.0f;
	m_camera_origin[1] = 0.0f;
	m_camera_origin[2] = 0.0f;
	z_scale = 0.0f;

	game::g_qeglobals->d_gridsize = 5;
	game::g_qeglobals->d_showgrid = true;

	game::printf_to_console("Entering message loop\n");

	mainframe->m_bDoLoop = true;
	SetTimer(mainframe->GetWindow(), 1u, 1000u, 0);
}

void __declspec(naked) hk_create_qe_children_stub()
{
	const static uint32_t retn_pt = 0x422989;
	__asm
	{
		pushad;
		push    edi;
		call	create_qe_children;
		add		esp, 4;
		popad;

		add		esp, 4;
		jmp		retn_pt;
	}
}

void cmainframe::update_windows(int nBits)
{
	if (!game::g_bScreenUpdates)
	{
		return;
	}

	if (game::glob::live_connected) 
	{
		components::remote_net::track_worldspawn_settings();
	}

	// check d3d device or we ASSERT in R_CheckHwnd_or_Device (!dx_device) when using floating windows
	if (!game::glob::d3d9_device)
	{
		return;
	}

#if 0
	if (nBits & (W_XY | W_XY_OVERLAY))
	{
		if (this->m_pXYWnd)
		{
			m_pXYWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
#endif

	if (nBits & W_CAMERA || ((nBits & W_CAMERA_IFON) && this->m_bCamPreview))
	{
		if (this->m_pCamWnd)
		{
			// redraw the camera when not running at realtime
			if(!game::glob::ccamwindow_realtime)
			{
				// Redraw the camera view
				m_pCamWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			
			// on update cam window through the 2d grid or something else
			if ((nBits & W_CAMERA_IFON) && this->m_bCamPreview || nBits < 0 || nBits == 3)
			{
			}

			// only update the remote cam when we actually move it, not when we update the cam window by doing something in the gridWnd etc.
			else if(game::glob::live_connected)
			{
				// Attempt to update the remote camera
				if(this->m_pCamWnd)
				{
					components::remote_net::cmd_send_camera_update(this->m_pCamWnd->camera.origin, this->m_pCamWnd->camera.angles);
				}
			}
		}
	}

#if 0
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
#endif
}


#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

BOOL is_mouse_outside_window(HWND hWnd)
{
	RECT  cr;
	DWORD msgpos = GetMessagePos();
	POINT pt = { GET_X_LPARAM(msgpos), GET_Y_LPARAM(msgpos) };

	ScreenToClient(hWnd, &pt);
	GetClientRect(hWnd, &cr);
	
	return !PtInRect(&cr, pt);
}


// typedef CFrameWnd::DefWindowProc
typedef LRESULT(__thiscall* wndproc_t)(cmainframe*, UINT Msg, WPARAM wParam, LPARAM lParam);
/* ------------------------- */ wndproc_t o_wndproc = reinterpret_cast<wndproc_t>(0x584D97);

/*
 * only active if clicked outside -> clicked imgui directly, not active if clicked into xywnd or camwnd and then into imgui
 */

// handle wm_char events for non-focused subwindows, see above msg
LRESULT __fastcall cmainframe::windowproc(cmainframe* pThis, [[maybe_unused]] void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if(ggui::is_ggui_initialized() && (Msg == WM_ACTIVATE || Msg == WM_SETFOCUS || Msg == WM_KILLFOCUS))
	{
		ImGuiIO& io = ImGui::GetIO();
		io.ClearInputKeys();

#ifdef DEBUG_KEYS
		game::printf_to_console("mainframe wndproc: reset keys (WM_ACTIVATE, SETFOCUS / KILLFOCUS");
#endif
	}

	if (Msg == WM_MOVE || Msg == WM_SIZE)
	{
		if(!IsIconic(pThis->GetWindow()))
		{
			// keep split-view cxy window maximized within the mainframe
			const auto prefs = game::g_PrefsDlg();

			if (prefs->m_nView == 1 && cmainframe::activewnd && cmainframe::activewnd->m_pZWnd)
			{
				RECT _rect;
				GetClientRect(pThis->m_hWnd, &_rect);

				int width = _rect.right - _rect.left;
				int height = _rect.bottom - _rect.top - WNDSTATUSBAR_HEIGHT;

				POINT _point = { 0,0 };
				ClientToScreen(pThis->m_hWnd, &_point);

				if(width == 0 || height == 0)
				{
					//__debugbreak();
					//return o_wndproc(pThis, Msg, wParam, lParam);

					width = 256;
					height = 256;
				}

				if (prefs->detatch_windows)
				{
					SetWindowPos(cmainframe::activewnd->m_pZWnd->GetWindow(), HWND_BOTTOM, _point.x, _point.y, width, height, 0); //SWP_FRAMECHANGED); //| SWP_NOZORDER | SWP_NOACTIVATE);
				}
				else
				{
					SetWindowPos(cmainframe::activewnd->m_pZWnd->GetWindow(), HWND_BOTTOM, 0, 0, width, height, 0); // SWP_NOACTIVATE SWP_NOZORDER |
				}
			}
		}
	}

	// ! do not set imgui context for every msg

	if (Msg == WM_SETCURSOR)
	{
		if (ggui::is_ggui_initialized())
		{
			// only above 10 fps (we might 'loose focus' on < 5 FPS, preventing imgui mouse input )
			//if(game::glob::frames_per_second <= 100)
			{
				// auto close mainframe menubar popups when the mouse leaves the cxy window
				if (cmainframe::activewnd && cmainframe::activewnd->m_pZWnd)
				{
					if (is_mouse_outside_window(cmainframe::activewnd->m_pZWnd->GetWindow()))
					{
						const auto imgui_context_old = ImGui::GetCurrentContext();

						IMGUI_BEGIN_CZWND;
						const auto context = ImGui::GetCurrentContext();

						// fix flickering background
						if(!GET_GUI(ggui::file_dialog)->is_active())
						{
							if (context->OpenPopupStack.Size > 0)
							{
								//printf("closing popup #%d\n", context->OpenPopupStack.Size - 1);
								ImGui::ClosePopupToLevel(context->OpenPopupStack.Size - 1, false);
							}
						}

						// restore context
						ImGui::SetCurrentContext(imgui_context_old);

						// reset keys when mouse leaves the mainframe
						ImGuiIO& io = ImGui::GetIO();
						io.ClearInputKeys();
					}
				}
			}

			// fix mouse cursors
			if(ImGui_ImplWin32_WndProcHandler(pThis->GetWindow(), Msg, wParam, lParam))
			{
				return true;
			}
		}
	}


	// only handle chars and keyup here (keyup not handled via event when mouse not within mainframe)
	if (Msg == WM_CHAR || Msg == WM_KEYUP)
	{
#ifdef DEBUG_KEYS
		game::printf_to_console("mainframe wndproc: %s %s", Msg == WM_CHAR ? "WM_CHAR" : Msg == WM_KEYDOWN ? "KEYDOWN" : "KEYUP",
			ggui::hotkey_dialog::cmdbinds_ascii_to_keystr(wParam).c_str());
#endif

		if (ggui::is_ggui_initialized())
		{
			IMGUI_BEGIN_CZWND;
			if (ImGui::GetIO().WantCaptureMouse)
			{
				ImGui_ImplWin32_WndProcHandler(pThis->GetWindow(), Msg, wParam, lParam);
				return true;
			}
			
			// reset io.KeysDown if cursor moved out of imgui window (fixes stuck keys)
			{
				ImGuiIO& io = ImGui::GetIO();
				io.ClearInputKeys();
			}
		}
	}
	
	// => CFrameWnd::DefWindowProc
	return o_wndproc(pThis, Msg, wParam, lParam);
}


// *
// | -------------------- MSG typedefs ------------------------
// *

namespace mainframe
{
	on_cmainframe_scroll	__on_mscroll;
	on_cmainframe_keydown	__on_keydown;
	on_cmainframe_keyup		__on_keyup;
	
}

typedef void(__thiscall* on_cmainframe_size)(cmainframe*, UINT, int, int);
	on_cmainframe_size		__on_size;

typedef void(__thiscall* on_cmainframe_on_destroy)(cmainframe*);
	on_cmainframe_on_destroy	__on_destroy;


// *
// | -------------------- Mouse Scroll ------------------------
// *

BOOL __fastcall cmainframe::on_mscroll(cmainframe* pThis, [[maybe_unused]] void* edx, UINT nFlags, SHORT zDelta, CPoint point)
{
	if (ggui::is_ggui_initialized())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui-cxy window
		if (auto gridwnd = GET_GUI(ggui::grid_dialog);
				 gridwnd->rtt_is_hovered())
		{
			return mainframe::__on_mscroll(pThis, nFlags, zDelta, gridwnd->rtt_get_cursor_pos_cpoint());
		}

		// if mouse is inside imgui-camera window
		if (auto camerawnd = GET_GUI(ggui::camera_dialog);
				 camerawnd->rtt_is_hovered())
		{
			float scroll_dir = zDelta <= 0 ? 1.0f : -1.0f;

			const static uint32_t CCamWnd__Scroll_func = 0x4248A0;
			__asm
			{
				pushad;

				mov		edi, cmainframe::activewnd; // yes .. :r
				push	cmainframe::activewnd;		// ^
				fld		scroll_dir;
				fstp    dword ptr[esp];
				call	CCamWnd__Scroll_func; // cleans the stack

				popad;
			}

			return 1;
		}

		// if mouse is inside texture window
		if(GET_GUI(ggui::texture_dialog)->rtt_is_hovered())
		{
			// CTexWnd::Scroll
			utils::hook::call<void(__cdecl)(std::int16_t _zDelta)>(0x45DD80)(zDelta);
		}

		if (ImGui::GetIO().WantCaptureMouse)
		{
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_MOUSEWHEEL, zDelta);
			return 1;
		}
	}
	
	return mainframe::__on_mscroll(pThis, nFlags, zDelta, point);
}


// *
// | ------------------------ Key ----------------------------
// *

void on_keydown_intercept(cmainframe* pThis, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	for(const auto& hotkey : ggui::cmd_addon_hotkeys_builtin)
	{
		if(hotkey.m_nKey == nChar)
		{
			unsigned int modifiers = 0;
			if (GetKeyState(VK_MENU) < 0)	 modifiers |= 2;
			if (GetKeyState(VK_CONTROL) < 0) modifiers |= 4;
			if (GetKeyState(VK_SHIFT) < 0)   modifiers |= 1;

			if ((hotkey.m_nModifiers & 0x7) == modifiers)
			{
				SendMessageA(pThis->GetWindow(), WM_COMMAND, hotkey.m_nCommand, 0);
				return;
			}
		}
	}

	for (const auto& hotkey : ggui::cmd_addon_hotkeys)
	{
		if (hotkey.m_nKey == nChar)
		{
			unsigned int modifiers = 0;
			if (GetKeyState(VK_MENU) < 0)	 modifiers |= 2;
			if (GetKeyState(VK_CONTROL) < 0) modifiers |= 4;
			if (GetKeyState(VK_SHIFT) < 0)   modifiers |= 1;

			if ((hotkey.m_nModifiers & 0x7) == modifiers)
			{
				components::command::execute(hotkey.m_strCommand);
				return;
			}
		}
	}

	mainframe::__on_keydown(pThis, nChar, nRepCnt, nFlags);
}

void __fastcall cmainframe::on_keydown(cmainframe* pThis, [[maybe_unused]] void* edx, UINT nChar, UINT nRepCnt, UINT nFlags)
{
#ifdef DEBUG_KEYS
	game::printf_to_console("mainframe keydown: %s", ggui::hotkey_dialog::cmdbinds_ascii_to_keystr(nChar).c_str());
#endif

	// fixes KP_ENTER getting stuck when mouse was outside the application
	// by setting the default focus to the camera window
	if (GetFocus() == pThis->GetWindow())
	{
		SetFocus(pThis->m_pCamWnd->GetWindow());
	}

	// there is one "bad" keydown on the first frame ..'O'
	if (nChar == 79 && !nRepCnt && !nFlags)
	{
		return;
	}

	if (ggui::is_ggui_initialized())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;

		// handles "special" keys like return and shift
		ImGui::HandleKeyIO(nullptr, WM_KEYDOWN, 0, nChar);

		if (ImGui::GetIO().WantTextInput)
		{
			return;
		}

		// if mouse is inside imgui-cxy window
		if (const auto	gridwnd = GET_GUI(ggui::grid_dialog);
						gridwnd->rtt_is_hovered())
		{
			on_keydown_intercept(pThis, nChar, nRepCnt, nFlags);
			return;
		}

		// handle imgui-camera window (only triggers when czwnd is focused)
		// cmainframe::on_keydown handles input if imgui-camera window is focused 
		if (const auto	camerawnd = GET_GUI(ggui::camera_dialog);
						camerawnd->rtt_is_hovered())
		{
			if (!components::physx_impl::get()->m_character_controller_enabled)
			{
				on_keydown_intercept(pThis, nChar, nRepCnt, nFlags);
			}

			return;
		}

		if (ImGui::GetIO().WantCaptureMouse)
		{
			ImGui::HandleKeyIO(nullptr, WM_KEYDOWN, 0, nChar);
		}

		// block specific keys from triggering hotkeys (eg. moving the camera via the arrow keys, because we use the up/down arrow to change xmodel previews)
		if (const auto	modelselector = GET_GUI(ggui::modelselector_dialog);
						modelselector->rtt_is_hovered())
		{
			// DownArrow / UpArrow
			if (nChar == VK_DOWN || nChar == VK_UP)
			{
				return;
			}
		}
	}

	on_keydown_intercept(pThis, nChar, nRepCnt, nFlags);
	//mainframe::__on_keydown(pThis, nChar, nRepCnt, nFlags);
}


void __stdcall cmainframe::on_keyup(cmainframe* pThis, UINT nChar)
{
#ifdef DEBUG_KEYS
	game::printf_to_console("mainframe keyup: %s", ggui::hotkey_dialog::cmdbinds_ascii_to_keystr(nChar).c_str());
#endif

	if (ggui::is_ggui_initialized())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;

		// handles "special" keys like return and shift
		ImGui::HandleKeyIO(nullptr, WM_KEYUP, 0, nChar);

		if (ImGui::GetIO().WantTextInput)
		{
			return;
		}

		// if mouse is inside imgui-cxy window
		if (const auto	gridwnd = GET_GUI(ggui::grid_dialog);
						gridwnd->rtt_is_hovered())
		{
			mainframe::__on_keyup(cmainframe::activewnd, nChar);
			return;
		}

		// handle imgui-camera window (only triggers when xywnd is focused)
		// cmainframe::on_keyup handles input if imgui-camera window is focused 
		if (const auto	camerawnd = GET_GUI(ggui::camera_dialog);
						camerawnd->rtt_is_hovered())
		{
			mainframe::__on_keyup(cmainframe::activewnd, nChar);
			return;
		}
	}

	mainframe::__on_keyup(pThis, nChar);
}


// *
// | ------------------------ Resize ----------------------------
// *

void __fastcall cmainframe::on_size(cmainframe* pThis, [[maybe_unused]] void* edx, UINT nFlags, int x, int y)
{
	if (!CZWND_POS_ONCE_ON_STARTUP)
	{
		const auto prefs = game::g_PrefsDlg();
		if (prefs->m_nView != 1)
		{
			// no need to set pos/size when not using split view
			CZWND_POS_ONCE_ON_STARTUP = true;
		}

		if (prefs->m_nView == 1 && cmainframe::activewnd && cmainframe::activewnd->m_pZWnd)
		{
			RECT _rect;
			GetClientRect(pThis->m_hWnd, &_rect);

			int width = _rect.right - _rect.left;
			int height = _rect.bottom - _rect.top - WNDSTATUSBAR_HEIGHT;

			POINT _point = { 0,0 };
			ClientToScreen(pThis->m_hWnd, &_point);

			if (width == 0 || height == 0)
			{
				//__debugbreak();
				//return __on_size(pThis, nFlags, x, y);

				width = 256;
				height = 256;
			}

			if (prefs->detatch_windows)
			{
				SetWindowPos(cmainframe::activewnd->m_pZWnd->GetWindow(), HWND_BOTTOM, _point.x, _point.y, width, height, /*SWP_FRAMECHANGED |*/ SWP_NOZORDER);  //| SWP_NOACTIVATE);
			}
			else
			{
				SetWindowPos(cmainframe::activewnd->m_pZWnd->GetWindow(), HWND_BOTTOM, 0, 0, width, height, SWP_NOZORDER); //  SWP_NOACTIVATE SWP_NOZORDER |
			}

			CZWND_POS_ONCE_ON_STARTUP = true;
		}
	}

	__on_size(pThis, nFlags, x, y);

	// immediately re-paint the rtt windows upon resizing (can make resizing smoother if FPS are high enough)
	if(cmainframe::activewnd)
	{
		if (const auto	hwnd = cmainframe::activewnd->m_pZWnd->GetWindow();
						hwnd != nullptr)
		{
			SendMessageA(hwnd, WM_PAINT, 0, 0);
		}

		if (const auto	hwnd = cmainframe::activewnd->m_pXYWnd->GetWindow();
						hwnd != nullptr)
		{
			SendMessageA(hwnd, WM_PAINT, 0, 0);
		}

		if (const auto	hwnd = cmainframe::activewnd->m_pCamWnd->GetWindow();
						hwnd != nullptr)
		{
			SendMessageA(hwnd, WM_PAINT, 0, 0);
		}

		if (const auto	hwnd = cmainframe::activewnd->m_pTexWnd->GetWindow();
						hwnd != nullptr)
		{
			SendMessageA(hwnd, WM_PAINT, 0, 0);
		}
	}
}


// *
// | ----------------------------------------------------------
// *

void __fastcall cmainframe::on_destroy(cmainframe* pThis)
{
	radiantapp::on_shutdown();
	__on_destroy(pThis);
}


// *
// | ----------------------------------------------------------
// *

void set_mainwindow_placement(HWND hwnd, WINDOWPLACEMENT* place)
{
	// always show window
	place->showCmd = 1;

	SetWindowPlacement(hwnd, place);
}

void __declspec(naked) set_windowplacement_stub()
{
	const static uint32_t retn_pt = 0x4225D3;

	__asm
	{
		pushad;

		push    eax;
		push    esi;
		call	set_mainwindow_placement;
		add		esp, 8;

		popad;

		jmp		retn_pt;
	}
}

void cmainframe::register_dvars()
{
	dvars::mainframe_show_console = dvars::register_bool(
		/* name		*/ "mainframe_show_console",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the console splitter-pane");

	dvars::mainframe_show_zview = dvars::register_bool(
		/* name		*/ "mainframe_show_zview",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the z-view splitter-pane");

	dvars::mainframe_show_toolbar = dvars::register_bool(
		/* name		*/ "mainframe_show_toolbar",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the toolbar");

	dvars::mainframe_show_menubar = dvars::register_bool(
		/* name		*/ "mainframe_show_menubar",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the menubar");
}

const COLORREF CFRAMEWND_BK_COL = RGB(44, 44, 44);

void cmainframe::hooks()
{
	// change mainframe background color (visible on startup)
	utils::hook::set(0x4217F3 + 1, CFRAMEWND_BK_COL);

	// hook continuous thread
	utils::hook(0x421A90, cmainframe::hk_routine_processing, HOOK_JUMP).install()->quick();

	// handle wm_char events for non-focused subwindows
	utils::hook(0x421A7B, cmainframe::windowproc, HOOK_CALL).install()->quick();

	// automatically load t5xradiant.prj
	utils::hook(0x422984, hk_create_qe_children_stub, HOOK_JUMP).install()->quick();

	// hook end of createclient
	utils::hook(0x4232EE, hk_on_createclient, HOOK_JUMP).install()->quick();

#ifdef HIDE_STATUSBAR
	utils::hook::nop(0x41F8E0, 5); // class init
	utils::hook::nop(0x420B04, 12 + 29 + 22); // create
	utils::hook::nop(0x4210ED, 59); // font stuff
#endif

	// hook SetWindowPlacement (Radiant::MainWindowPlace) in OnCreateClient to fix minimize issue
	utils::hook::nop(0x4225CB, 8);
		 utils::hook(0x4225CB, set_windowplacement_stub, HOOK_JUMP).install()->quick();
	
	// *
	// detour cmainframe member functions to get imgui input

	mainframe::__on_mscroll	= reinterpret_cast<mainframe::on_cmainframe_scroll> (utils::hook::detour(0x42B850, cmainframe::on_mscroll, HK_JUMP));
	mainframe::__on_keydown	= reinterpret_cast<mainframe::on_cmainframe_keydown>(utils::hook::detour(0x422370, cmainframe::on_keydown, HK_JUMP));
	mainframe::__on_keyup	= reinterpret_cast<mainframe::on_cmainframe_keyup>  (utils::hook::detour(0x422270, cmainframe::on_keyup, HK_JUMP));
			   __on_size	= reinterpret_cast<on_cmainframe_size>				(utils::hook::detour(0x423310, cmainframe::on_size, HK_JUMP));
			   __on_destroy = reinterpret_cast<on_cmainframe_on_destroy>		(utils::hook::detour(0x421C60, cmainframe::on_destroy, HK_JUMP));
	
	// *
	// | ------------------------ Commands ----------------------------
	// *
	
	components::command::register_command("menubar_show"s, [](std::vector<std::string> args)
	{
		const auto menubar = LoadMenu(cmainframe::activewnd->m_pModuleState->m_hCurrentInstanceHandle, MAKEINTRESOURCE(0xD6)); // 0xD6 = IDR_MENU_QUAKE3

		SetMenu(cmainframe::activewnd->GetWindow(), menubar);
		
		ggui::mainframe_menubar_enabled = true;
		dvars::set_bool(dvars::mainframe_show_menubar, true);
	});

	components::command::register_command("menubar_hide"s, [](std::vector<std::string> args)
	{
		// destroy the menu or radiant crashes on shutdown when its trying to get the menubar style
		if (const auto menu = utils::hook::call<CMenu * (__fastcall)(cmainframe*)>(0x42EE20)(cmainframe::activewnd); menu) // GetMenuFromHandle
		{
			// CMenu::DestroyMenu
			utils::hook::call<void(__fastcall)(CMenu*)>(0x58A908)(menu);
		}

		SetMenu(cmainframe::activewnd->GetWindow(), nullptr);
		
		ggui::mainframe_menubar_enabled = false;
		dvars::set_bool(dvars::mainframe_show_menubar, false);
	});
}
