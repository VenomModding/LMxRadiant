#pragma once

struct camera_s
{
	int	width;
	int height;
	bool timing;
	char pad_timing[3];
	game::vec3_t origin;
	game::vec3_t angles;
	int draw_mode;
	game::vec3_t color;
	game::vec3_t forward;
	game::vec3_t right;
	game::vec3_t up;
	game::vec3_t vup;
	game::vec3_t vpn;
	game::vec3_t vright;
};

class ccamwnd : public CWnd
{
private:
	// Do not allow this class to be instanced
	ccamwnd() = delete;
	~ccamwnd() = delete;

public:
	void* m_pUnkown;
	camera_s camera;
	int m_nCambuttonstate;
	CPoint m_ptButton;
	int unkown01;
	CPoint m_ptCursor;
	CPoint m_ptLastCursor;

	//
	static ccamwnd *activewnd;

	void					mouse_control(float dtime);

	static void				main();
	static BOOL WINAPI		windowproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	//static void			on_endframe();

	static void __fastcall	on_lbutton_down(ccamwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point);
	static void __fastcall	on_lbutton_up(ccamwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point);

	static void __fastcall	on_rbutton_down(ccamwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point);
	static void __fastcall	on_rbutton_up(ccamwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point);

	static void __fastcall	on_mouse_move(ccamwnd* pThis, [[maybe_unused]] void* edx, UINT nFlags, CPoint point);

	static void __stdcall	on_keydown(UINT nChar, UINT nRepCnt, UINT nFlags);
	static void __stdcall	on_keyup(UINT nChar, UINT nRepCnt, UINT nFlags);
};
STATIC_ASSERT_OFFSET(ccamwnd, camera.origin, 0x64); // diff. offset then bo1
STATIC_ASSERT_OFFSET(ccamwnd, camera.angles, 0x70); // ^
STATIC_ASSERT_OFFSET(ccamwnd, m_nCambuttonstate, 0xD4);