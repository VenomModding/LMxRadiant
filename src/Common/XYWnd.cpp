#include "STDInclude.hpp"

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef void(__thiscall* on_cxywnd_msg)(CXYWnd*, UINT, CPoint);

	// mouse scrolling handled in CMainFrame
	on_cxywnd_msg __on_lbutton_down;
	on_cxywnd_msg __on_lbutton_up;
	on_cxywnd_msg __on_rbutton_down;
	on_cxywnd_msg __on_rbutton_up;
	on_cxywnd_msg __on_mouse_move;


typedef void(__stdcall* on_cxywnd_key)(UINT nChar, UINT nRepCnt, UINT nFlags);

	on_cxywnd_key __on_keydown;
	on_cxywnd_key __on_keyup;

// *
// | ----------------- Left Mouse Button ---------------------
// *

void __fastcall CXYWnd::on_lbutton_down(CXYWnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_LBUTTONDOWN);
	}
	else
    {
        return __on_lbutton_down(pThis, nFlags, point);
    }
}

void __fastcall CXYWnd::on_lbutton_up(CXYWnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_LBUTTONUP);
	}
	else
    {
        return __on_lbutton_up(pThis, nFlags, point);
    }
}

// *
// | ----------------- Right Mouse Button ---------------------
// *

void __fastcall CXYWnd::on_rbutton_down(CXYWnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_RBUTTONDOWN);
	}
	else
    {
        return __on_rbutton_down(pThis, nFlags, point);
    }
}

void __fastcall CXYWnd::on_rbutton_up(CXYWnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(pThis->GetWindow(), WM_RBUTTONUP);
	}
	else
    {
        return __on_rbutton_up(pThis, nFlags, point);
    }
}

void __fastcall CXYWnd::on_mouse_move(CXYWnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		return __on_mouse_move(pThis, nFlags, point);
	}
}

void __stdcall CXYWnd::on_keydown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(CMainFrame::ActiveWindow->m_pXYWnd->GetWindow(), WM_KEYDOWN, 0, nChar);
	}
	else
	{
		return __on_keydown(nChar, nRepCnt, nFlags);
	}
}

void __stdcall CXYWnd::on_keyup(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	ImGui::SetCurrentContext(Game::Globals::_context_cxy);
	
	if (ImGui::GetIO().WantCaptureMouse)
	{
		Game::ImGui_HandleKeyIO(CMainFrame::ActiveWindow->m_pXYWnd->GetWindow(), WM_KEYUP, 0, nChar);
	}
	else
	{
		return __on_keyup(nChar, nRepCnt, nFlags);
	}
}


void CXYWnd::on_endframe()
{
	Game::Globals::gui_present.CXYWnd = true;
	Game::R_EndFrame();
}

void CXYWnd::main()
{
	// ! no need to hook windowproc

	// endframe hook to set imgui present bool
	Utils::Hook(0x465C0E, CXYWnd::on_endframe, HOOK_CALL).install()->quick();

	__on_lbutton_down = reinterpret_cast<on_cxywnd_msg>(Utils::Hook::Detour(0x463F70, CXYWnd::on_lbutton_down, HK_JUMP));
	__on_lbutton_up = reinterpret_cast<on_cxywnd_msg>(Utils::Hook::Detour(0x464860, CXYWnd::on_lbutton_up, HK_JUMP));

	__on_rbutton_down = reinterpret_cast<on_cxywnd_msg>(Utils::Hook::Detour(0x4647B0, CXYWnd::on_rbutton_down, HK_JUMP));
	__on_rbutton_up = reinterpret_cast<on_cxywnd_msg>(Utils::Hook::Detour(0x464990, CXYWnd::on_rbutton_up, HK_JUMP));

	__on_mouse_move = reinterpret_cast<on_cxywnd_msg>(Utils::Hook::Detour(0x464B10, CXYWnd::on_mouse_move, HK_JUMP));

	__on_keydown = reinterpret_cast<on_cxywnd_key>(Utils::Hook::Detour(0x465C90, CXYWnd::on_keydown, HK_JUMP));
	__on_keyup = reinterpret_cast<on_cxywnd_key>(Utils::Hook::Detour(0x46E510, CXYWnd::on_keyup, HK_JUMP));
}