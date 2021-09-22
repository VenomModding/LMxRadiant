#include "std_include.hpp"

zwnd_s* zwnd = reinterpret_cast<zwnd_s*>(0x241A590);

typedef  void(__thiscall* CamWnd__DropModelsToPlane_t)(ccamwnd*, int x, int y, int buttons);
const auto CamWnd__DropModelsToPlane = reinterpret_cast<CamWnd__DropModelsToPlane_t>(0x403D30);

// *
// | -------------------- gui i/o ------------------------
// *

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * (czwnd member functions are only used if the cz subwindow is focused)
 * - Directly clicking onto an imgui window will not focus the subwindow behind it
 * - IO will instead be handled by cmainframe member functions
 * + Mouse scrolling handled by cmainframe::on_mscroll
 * + Char events handled by czwnd::wndproc / cxywnd::wndproc / ccamwnd::wndproc or cmainframe::windowproc (depends on focused window)
 */


typedef void(__thiscall* on_czwnd_msg)(czwnd*, UINT, CPoint);
	on_czwnd_msg __on_lbutton_down;
	on_czwnd_msg __on_lbutton_up;
	on_czwnd_msg __on_rbutton_down;
	on_czwnd_msg __on_rbutton_up;
	on_czwnd_msg __on_mbutton_down;
	on_czwnd_msg __on_mbutton_up;
	on_czwnd_msg __on_mouse_move;

// *
// | ----------------- Left Mouse Button ---------------------
// *

void __fastcall czwnd::on_lbutton_down(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;
		

		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_lbutton_down(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			const auto ccam = cmainframe::activewnd->m_pCamWnd;
			ccam->m_ptLastCursor = ggui::rtt_camerawnd.cursor_pos_pt;
			CamWnd__DropModelsToPlane(ccam, ccam->m_ptLastCursor.x, ccam->camera.height - ccam->m_ptLastCursor.y - 1, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_LBUTTONDOWN);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_lbutton_down(pThis, nFlags, point);
}

void __fastcall czwnd::on_lbutton_up(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_lbutton_up(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			ccamwnd::mouse_up(cmainframe::activewnd->m_pCamWnd, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_LBUTTONUP);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_lbutton_up(pThis, nFlags, point);
}


// *
// | ----------------- Right Mouse Button ---------------------
// *

void __fastcall czwnd::on_rbutton_down(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_rbutton_down(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			const auto ccam = cmainframe::activewnd->m_pCamWnd;
			CamWnd__DropModelsToPlane(ccam, ggui::rtt_camerawnd.cursor_pos_pt.x, ccam->camera.height - ggui::rtt_camerawnd.cursor_pos_pt.y - 1, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_RBUTTONDOWN);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_rbutton_down(pThis, nFlags, point);
}

void __fastcall czwnd::on_rbutton_up(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_rbutton_up(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			const auto ccam = cmainframe::activewnd->m_pCamWnd;
			const int cursor_point_y = ccam->camera.height - ggui::rtt_camerawnd.cursor_pos_pt.y - 1;

			// context menu
			const static uint32_t CCamWnd__ContextMenu_Func = 0x404D40;
			__asm
			{
				pushad;

				push	cursor_point_y;
				push	ggui::rtt_camerawnd.cursor_pos_pt.x;
				mov     ebx, ccam;
				call	CCamWnd__ContextMenu_Func; // cleans the stack

				popad;
			}

			ccamwnd::mouse_up(cmainframe::activewnd->m_pCamWnd, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_RBUTTONUP);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_rbutton_up(pThis, nFlags, point);
}



// *
// | ----------------- Middle Mouse Button ---------------------
// *

void __fastcall czwnd::on_mbutton_down(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_mbutton_down(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			const auto ccam = cmainframe::activewnd->m_pCamWnd;
			CamWnd__DropModelsToPlane(ccam, ggui::rtt_camerawnd.cursor_pos_pt.x, ccam->camera.height - ggui::rtt_camerawnd.cursor_pos_pt.y - 1, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_MBUTTONDOWN);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_mbutton_down(pThis, nFlags, point);
}

void __fastcall czwnd::on_mbutton_up(czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui grid window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_mbutton_up(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			ccamwnd::mouse_up(cmainframe::activewnd->m_pCamWnd, nFlags);
			return;
		}
		

		// if mouse cursor above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// imgui input -> don't pass input to the czwindow
			ImGui::HandleKeyIO(pThis->GetWindow(), WM_MBUTTONUP);
			return;
		}
	}

	// we dont need the original functionality
	// pass input to the czwnd
	//__on_mbutton_up(pThis, nFlags, point);
}



// *
// | ----------------- Mouse Move ---------------------
// *

void __fastcall czwnd::on_mouse_move([[maybe_unused]] czwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, [[maybe_unused]] CPoint point)
{
	if (ggui::cz_context_ready())
	{
		// set cz context (in-case we use multiple imgui context's)
		IMGUI_BEGIN_CZWND;


		// if mouse is inside imgui-cxy window
		if (ggui::rtt_gridwnd.window_hovered)
		{
			xywnd::__on_mouse_move(cmainframe::activewnd->m_pXYWnd, nFlags, ggui::rtt_gridwnd.cursor_pos_pt);
			return;
		}

		// if mouse is inside imgui-camera window
		if (ggui::rtt_camerawnd.window_hovered)
		{
			const auto ccam = cmainframe::activewnd->m_pCamWnd;
			const int cursor_point_y = ccam->camera.height - ggui::rtt_camerawnd.cursor_pos_pt.y - 1;

			const static uint32_t CCamWnd__Cam_MouseMoved_Func = 0x404FC0;
			__asm
			{
				pushad;

				mov		eax, nFlags;
				push	cursor_point_y;
				push	ggui::rtt_camerawnd.cursor_pos_pt.x;
				mov		ecx, ccam;
				call	CCamWnd__Cam_MouseMoved_Func; // cleans the stack

				popad;
			}

			return;
		}

		
		// block czwindow input if mouse cursor is above any cz-imgui window
		if (ImGui::GetIO().WantCaptureMouse)
		{
			return;
		}
	}

	// we dont need the original functionality
	//__on_mouse_move(pThis, nFlags, point);
}



// *
// | ----------------- Windowproc ---------------------
// *

LRESULT WINAPI czwnd::windowproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// we only need the char event
	if (Msg == WM_CHAR)
	{
		if (ggui::cz_context_ready())
		{
			// set cz context (in-case we use multiple imgui context's)
			IMGUI_BEGIN_CZWND;

			// if mouse is inside imgui-cxy window
			if (ggui::rtt_gridwnd.window_hovered)
			{
				return DefWindowProcA(hWnd, Msg, wParam, lParam);
			}

			// if mouse is inside imgui-camera window :: ??
			if (ggui::rtt_camerawnd.window_hovered)
			{
				return DefWindowProcA(hWnd, Msg, wParam, lParam);
			}
			
			// handle char inputs when cz window is focused
			if (ImGui::GetIO().WantCaptureMouse)
			{
				ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);
				return true;
			}
		}
	}

	return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

void __declspec(naked) czwindowproc_stub()
{
	const static uint32_t AfxRegisterClass_Func = 0x58A0A1;
	const static uint32_t retn_pt = 0x46F365;
	__asm
	{
		mov     dword ptr[esp + 10h], offset czwnd::windowproc;
		call	AfxRegisterClass_Func;

		jmp		retn_pt;
	}
}



// *
// | ----------------- Misc ---------------------
// *

__declspec(naked) void set_czwnd_style()
{
	const static uint32_t retn_pt = 0x46F394;
	__asm
	{
		mov     dword ptr[eax + 20h], WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD;
		jmp		retn_pt;
	}
}

// *
// *

void czwnd::main()
{
	// change window style -> child window
	utils::hook::nop(0x46F38B, 9);
		 utils::hook(0x46F38B, set_czwnd_style, HOOK_JUMP).install()->quick();

	// disable z-drawing in on_paint
	utils::hook::nop(0x46EF23, 5);

	// use texture background color as background color (0xFC to 0xEC because g_qeglobals.d_savedinfo.colors[1] to [0])
	utils::hook::set<BYTE>(0x46EF17 + 1, 0xEC);

	// custom windowproc
	utils::hook::nop(0x46F35C, 9);
		 utils::hook(0x46F35C, czwindowproc_stub, HOOK_JUMP).install()->quick();

	__on_lbutton_down	= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46EDA0, czwnd::on_lbutton_down, HK_JUMP));
	__on_lbutton_up		= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46F220, czwnd::on_lbutton_up, HK_JUMP));
	
	__on_rbutton_down	= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46EE60, czwnd::on_rbutton_down, HK_JUMP));
	__on_rbutton_up		= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46F2A0, czwnd::on_rbutton_up, HK_JUMP));

	__on_mbutton_down	= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46EE00, czwnd::on_mbutton_down, HK_JUMP));
	__on_mbutton_up		= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46F260, czwnd::on_mbutton_up, HK_JUMP));

	__on_mouse_move		= reinterpret_cast<on_czwnd_msg>(utils::hook::detour(0x46EFC0, czwnd::on_mouse_move, HK_JUMP));

	// no need to detour key up/down or scroll, these call the mainframe variant anyway, so just handle them there
}