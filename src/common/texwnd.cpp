#include "std_include.hpp"

texwnd_s* g_texwnd = reinterpret_cast<texwnd_s*>(0x25D7990);
std::vector<std::vector<std::string>> texwnd_vector_of_favourites;

const char* ctexwnd::get_name_for_selection()
{
	const auto selection = reinterpret_cast<game::MaterialDef*>(&game::g_qeglobals->random_texture_stuff[2100 * game::g_qeglobals->current_edit_layer]);

	if(selection && selection->radMtl && selection->radMtl->name)
	{
		return selection->radMtl->name;
	}

	return nullptr;
}

void ctexwnd::add_selected_to_favourite_list(int list_id)
{
	if(list_id < static_cast<int>(texwnd_vector_of_favourites.size()))
	{
		if (const auto	mat_name = ctexwnd::get_name_for_selection();
						mat_name)
		{
			// do not add dupes
			if (std::ranges::find(texwnd_vector_of_favourites[list_id].begin(), texwnd_vector_of_favourites[list_id].end(), mat_name) == texwnd_vector_of_favourites[list_id].end())
			{
				texwnd_vector_of_favourites[list_id].emplace_back(mat_name);
			}
		}
	}
}

void ctexwnd::remove_selected_from_favourite_list(int list_id)
{
	if (list_id < static_cast<int>(texwnd_vector_of_favourites.size()))
	{
		if (const auto	mat_name = ctexwnd::get_name_for_selection();
						mat_name)
		{
			const auto itr = std::ranges::find(texwnd_vector_of_favourites[list_id].begin(), texwnd_vector_of_favourites[list_id].end(), mat_name);
			if (itr != texwnd_vector_of_favourites[list_id].end())
			{
				texwnd_vector_of_favourites[list_id].erase(itr);
			}
		}
	}
}

void ctexwnd::write_favourite_list(int list_id)
{
	if (dvars::fs_homepath = game::Dvar_FindVar("fs_homepath");
		dvars::fs_homepath)
	{
		std::string dir_path = dvars::fs_homepath->current.string;
					dir_path += R"(\IW3xRadiant\texture_favourites\)";

		std::filesystem::create_directories(dir_path);
		if (std::filesystem::exists(dir_path))
		{
			bool found_list = false;
			std::filesystem::path list_path;

			// find list
			for (auto& entry : std::filesystem::directory_iterator(dir_path))
			{
				if (entry.path().extension() == ".txt")
				{
					std::ifstream file;
					file.open(entry.path());

					if (file.is_open())
					{
						std::string input;
						int line_num = 0;

						// read line by line
						while (std::getline(file, input))
						{
							if (!line_num && input.find("// texture favourites generated by iw3xo-radiant") == std::string::npos)
							{
								// invalid file
								break;
							}

							// skip first line
							if (!line_num)
							{
								line_num++;
								continue;
							}

							if (input == texwnd_vector_of_favourites[list_id][0])
							{
								found_list = true;
								list_path = entry.path();
								break;
							}
						}

						file.close();
					}
				}

				if (found_list)
				{
					break;
				}
			}

			if (found_list)
			{
				std::ofstream ofile;
				ofile.open(list_path);

				if (ofile.is_open())
				{
					ofile << "// texture favourites generated by iw3xo-radiant" << std::endl;
					ofile << texwnd_vector_of_favourites[list_id][0] << std::endl;

					int line_num = 0;
					for (const auto& entry : texwnd_vector_of_favourites[list_id])
					{
						// skip first line
						if (!line_num)
						{
							line_num++;
							continue;
						}

						ofile << entry << std::endl;
					}

					ofile.close();
				}
			}
			else
			{
				game::printf_to_console("[ERR][TEX] Failed to find favourite list '%s' in 'bin/IW3xRadiant/texture_favourites'", texwnd_vector_of_favourites[list_id][0].c_str());
			}
		}
	}
}

void ctexwnd::apply_favourite(int list)
{
	if (list >= static_cast<int>(texwnd_vector_of_favourites.size()))
	{
		game::printf_to_console("[ERR] ctexwnd::apply_favourite - invalid list");
		return;
	}

	const auto texwndglob_textures = reinterpret_cast<game::qtexture_s*>(*(DWORD*)0x25E79A8);

	// hide all
	game::qtexture_s* tex = texwndglob_textures;
	for (; tex; tex = tex->prev)
	{
		tex->is_in_use = false;
	}

	// show fav's
	tex = texwndglob_textures;
	for (; tex; tex = tex->prev)
	{
		int idx = 0;
		for (const auto& str : texwnd_vector_of_favourites[list])
		{
			if(!idx)
			{
				idx++;
				continue;
			}

			if (utils::string_equals(tex->name, str.c_str()))
			{
				tex->is_in_use = true;
				break;
			}

			idx++;
		}
	}
}

void ctexwnd::load_favourites()
{
	const auto gui = GET_GUI(ggui::texture_dialog);
	gui->set_favourite_index(0);
	gui->set_favourite_str("");
	texwnd_vector_of_favourites.clear();

	g_texwnd->nPos[0].nPos_current = 0; // scroll to top
	cdeclcall(void, 0x45B850); // Texture_ShowInuse

	dvars::fs_homepath = game::Dvar_FindVar("fs_homepath");
	if (dvars::fs_homepath)
	{
		std::ifstream file;
		
		std::string filepath = dvars::fs_homepath->current.string;
					filepath += R"(\IW3xRadiant\texture_favourites\)";

		if (std::filesystem::exists(filepath))
		{
			game::printf_to_console("[TEX] Loading texture favourites .. \n");

			for (auto& entry : std::filesystem::directory_iterator(filepath))
			{
				if (entry.path().extension() == ".txt")
				{
					file.open(entry.path());
					if (file.is_open())
					{
						std::vector<std::string> curr_vec;
						curr_vec.reserve(30);

						std::string input;
						int line_num = 0;

						// read line by line
						while (std::getline(file, input))
						{
							if (!line_num && input.find("// texture favourites generated by iw3xo-radiant") == std::string::npos)
							{
								// invalid file
								break;
							}

							// skip first line
							if (!line_num)
							{
								line_num++;
								continue;
							}

							// do not add dupes
							if (std::ranges::find(curr_vec.begin(), curr_vec.end(), input) == curr_vec.end())
							{
								curr_vec.emplace_back(input);
							}

							line_num++;
						}

						if (line_num)
						{
							texwnd_vector_of_favourites.emplace_back(curr_vec);
						}

						file.close();
					}
				}
			}
		}
	}
}

/**
 * @brief			selects a texture in the texture window based on mouse coordinates
 * @param point		local coordinates
 */
void ctexwnd::select_texture(CPoint point)
{
	game::Texwnd_SelectMaterial(point.x, g_texwnd->nPos[game::g_qeglobals->current_edit_layer].nPos_current + point.y);
}

/**
 * @brief			calls og internal OnButtonDown function
 * @param nFlags	
 */
void ctexwnd::on_mousebutton_down(UINT nFlags)
{
	auto point = GET_GUI(ggui::texture_dialog)->rtt_get_cursor_pos_cpoint();
	const static uint32_t CTexWnd_OnButtonDown_func = 0x45C9A0;
	__asm
	{
		pushad;
		push	point.y;
		push	point.x;
		mov		eax, nFlags;
		call	CTexWnd_OnButtonDown_func;
		add     esp, 8;
		popad;
	}
}

void ctexwnd::on_mousebutton_up(UINT nFlags)
{
	if ((nFlags & (MK_MBUTTON | MK_RBUTTON | MK_LBUTTON)) == 0)
	{
		// CTexWnd::OnButtonUp
		cdeclcall(void, 0x45CA30);

		ReleaseCapture();
	}
}

void ctexwnd::on_mousemove(UINT nFlags)
{
	const static uint32_t CTexWnd__OnMouseFirst_func = 0x45CA60;
	__asm
	{
		pushad;
		mov		eax, nFlags;
		call	CTexWnd__OnMouseFirst_func;
		popad;
	}
}

BOOL __fastcall ctexwnd::on_paint(ctexwnd* pThis)
{
	PAINTSTRUCT Paint;
	BeginPaint(pThis->GetWindow(), &Paint);

	// R_CheckHwnd_or_Device
	if (!game::R_SetupRendertarget_CheckDevice(pThis->GetWindow()))
	{
		return EndPaint(pThis->GetWindow(), &Paint);
	}

	if(GET_GUI(ggui::texture_dialog)->is_active())
	{
		game::R_BeginFrame();
		game::R_Clear(7, game::g_qeglobals->d_savedinfo.colors[0], 1.0f, 0);

		// SetProjection 2D
		cdeclcall(game::GfxCmdHeader*, 0x4FD390);

		// R_DrawTexWnd
		cdeclcall(void, 0x45D0F0);

		game::R_EndFrame();
		game::R_IssueRenderCommands(-1);
		game::R_SortMaterials();
	}

	// R_CheckTargetWindow
	game::R_CheckTargetWindow(pThis->GetWindow());

	int& random_dword01 = *reinterpret_cast<int*>(0x25D5B88);
	int& random_dword02 = *reinterpret_cast<int*>(0x242293C);
	int& random_dword03 = *reinterpret_cast<int*>(0x2422940);

	if (!random_dword01)
	{
		__debugbreak();
	}

	random_dword03 = random_dword02;

	// nice meme IW
	return EndPaint(pThis->GetWindow(), &Paint);
}

bool texwnd_textfilter(const char* iter_material_name)
{
	const auto tex = GET_GUI(ggui::texture_dialog);
	const auto& filter = tex->get_filter();

	if (tex->get_filter_length())
	{
		if (std::string(iter_material_name).find(filter.InputBuf) != std::string::npos)
		{
			return true;
		}
	}

	return false;
}

// asm helper function
int texwnd_get_filter_length()
{
	return GET_GUI(ggui::texture_dialog)->get_filter_length();
}

void __declspec(naked) texwnd_listmaterials_intercept()
{
	const static uint32_t no_filter_pt = 0x45BD89;
	const static uint32_t break_pt = 0x45BD11;
	const static uint32_t goto_pt = 0x45BD8F;
	__asm
	{
		// test if textbox filter is enabled (textlen > 0)
		call	texwnd_get_filter_length;
		test	eax, eax;
		jz		NO_FILTER;

		// filter active
		mov     eax, [ebp - 28h]; // iter_material_name
		push	eax;
		call	texwnd_textfilter;
		add		esp, 4;

		test	al, al;
		jnz		BREAK_F;
		jmp		goto_pt;

	BREAK_F:
		jmp		break_pt;

	NO_FILTER:
		jmp		no_filter_pt;
	}
}

// *
// detours

// CMainFrame::OnViewTexture
void on_viewtextures_command()
{
	const auto tex = GET_GUI(ggui::texture_dialog);

	if(tex->is_inactive_tab() && tex->is_active())
	{
		tex->set_bring_to_front(true);
		return;
	}

	tex->toggle();
}

// CMainFrame::OnTexturesShowinuse
void on_textures_show_in_use_command()
{
	const auto tex = GET_GUI(ggui::texture_dialog);
	tex->set_bring_to_front(true);
	tex->open();

	// Texture_ShowInuse
	cdeclcall(void, 0x45B850);
}


// CMainFrame::OnTexturesShowall (intercept: no logic besides showing the menu)
void on_textures_show_all_command_intercept()
{
	const auto tex = GET_GUI(ggui::texture_dialog);
	tex->set_bring_to_front(true);
	tex->open();
}

void __declspec(naked) on_textures_show_all_command_stub()
{
	const static uint32_t sub_453E50 = 0x453E50;
	const static uint32_t retn_addr = 0x42B445;
	__asm
	{
		pushad;
		call	on_textures_show_all_command_intercept;
		popad;
		
		call    sub_453E50; // overwritten
		jmp     retn_addr; // jump back to "test al, al"
	}
}

void set_default_texture()
{
	cdeclcall(void, 0x45B650); // Texture_ResetPosition
}

void __declspec(naked) set_default_texture_stub()
{
	const static uint32_t retn_addr = 0x420025;

	__asm
	{
		pushad;
		call	set_default_texture;
		popad;

		jmp		retn_addr;
	}
}

// called from config::load_dvars()
void ctexwnd::init()
{
	ctexwnd::load_favourites();
}

void ctexwnd::register_dvars()
{
	dvars::gui_texwnd_draw_scrollbar = dvars::register_bool(
		/* name		*/ "gui_texwnd_draw_scrollbar",
		/* default	*/ true,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "Draw texturewindow scrollbar");

	dvars::gui_texwnd_draw_scrollpercent = dvars::register_bool(
		/* name		*/ "gui_texwnd_draw_scrollpercent",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "Draw texturewindow scroll in percent (position in % / 100%)");
}

void ctexwnd::hooks()
{
	// do not set parent window for texture window
	utils::hook::nop(0x4228C1, 8);

	// ignore crashing texture searchbar destructor :x
	utils::hook::nop(0x627990, 5);
	utils::hook::set<BYTE>(0x627990, 0xC3);

	utils::hook::nop(0x45BCDD, 8);
	utils::hook(0x45BCDD, texwnd_listmaterials_intercept, HOOK_JUMP).install()->quick();

	// disable texture tab insertion in entitywnd :: CTabCtrl::InsertItem(&g_wndTabsEntWnd, 1u, 1u, "&Textures", 0, 0);
	utils::hook::nop(0x49672A, 23);

	// TODO! :: why does the default OnPaint function induces lag on all windows (even outside radiant) when calling it 250 times a second?
	// -- rewritten one runs fine (EndPaint?)
	utils::hook::detour(0x45DB20, ctexwnd::on_paint, HK_JUMP);

	// detour the view textures hotkey (CMainFrame::OnViewTexture) to open the imgui texture window
	utils::hook::detour(0x424440, on_viewtextures_command, HK_JUMP);

	// detour the show textures in use hotkey (CMainFrame::OnTexturesShowinuse) to open the imgui texture window
	utils::hook::detour(0x424B20, on_textures_show_in_use_command, HK_JUMP);

	// detour fails here .. so doing it manually -> make the show all textures hotkey (CMainFrame::OnTexturesShowall) open the imgui texture window
	utils::hook(0x42B440, on_textures_show_all_command_stub, HOOK_JUMP).install()->quick();

	// set default selected texture to caulk
	utils::hook::nop(0x41FC69, 6);
	utils::hook(0x41FC69, set_default_texture_stub, HOOK_JUMP).install()->quick();

	// disable og context menu
	utils::hook::nop(0x45CA54, 5);

	// replace $default material with caulk
	const char* replace_with_caulk = "caulk";
	utils::hook::set<DWORD*>(0x4756EF + 1, (DWORD*)replace_with_caulk);
}