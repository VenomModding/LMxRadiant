#pragma once
#include "imgui_internal.h"

namespace ImGui
{
	ImGuiID FindNodeByID(ImGuiID id);
	void	DockBuilderDockWindow_FirstUseOrSaved(const char* window_name, ImGuiID node_id);
	bool	IsItemHoveredDelay(float delay_in_seconds);
	void	HandleKeyIO(HWND hwnd, UINT key, SHORT zDelta = 0, UINT nChar = 0);
	ImVec4	ToImVec4(const float* in);
	void	HelpMarker(const char* desc);
	void	PushFontFromIndex(int font);

	void	debug_table_entry_vec3(const char* label, const float* vec3);
	void	debug_table_entry_float(const char* label, const float* val);
	void	debug_table_entry_int(const char* label, const int val);
	void	debug_table_entry_int2(const char* label, const int* val);
	
	void	StyleColorsDevgui(ImGuiStyle* = NULL);
	
}
