#include "std_include.hpp"

const int MAX_MATCHES_TO_SHOW = 12;

namespace ggui
{
	void console_dialog::strtrim(char* s)
	{
		char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0;
	}

	void console_dialog::clear_log()
	{
		for (int i = 0; i < m_items.Size; i++)
		{
			free(m_items[i]);
		}

		m_items.clear();
	}

	void console_dialog::addline_no_format(const char* text)
	{
		// check for multiline prints (the current imgui clipper only works with widgets of the same height, so 1 line in this case)
		const auto lines = utils::explode(text, '\n');
		for (auto& line : lines)
		{
			m_items.push_back(_strdup(line.c_str()));

			std::string t_line = line;

			if (utils::starts_with(t_line, "[ERR]", true)
				|| utils::starts_with(t_line, "^1ERROR:", true)
				|| utils::starts_with(t_line, "^1", true)
				|| utils::starts_with(t_line, "ERROR:")
				|| utils::starts_with(t_line, "Error:"))
			{
				utils::ltrim(t_line);

				ImGuiToast toast(ImGuiToastType_Error, 3000);
				toast.set_title("Error");
				toast.set_content(t_line.c_str());
				ImGui::InsertNotification(toast);
			}
		}
	}

	void console_dialog::addline(const char* fmt, ...) IM_FMTARGS(2)
	{
		char buf[1024];
		va_list args;

		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);

		// check for multiline prints (the current imgui clipper only works with widgets of the same height, so 1 line in this case)
		const auto lines = utils::explode(buf, '\n');
		for (auto& line : lines)
		{
			m_items.push_back(_strdup(line.c_str()));
		}
	}

	void console_dialog::draw_text_with_color(const char* text, int index)
	{
		ImVec4 color;

		bool has_color = false;
		std::string item_s = text;

		if (   utils::starts_with(item_s, "[ERR]", true)
			|| utils::starts_with(item_s, "^1ERROR:", true)
			|| utils::starts_with(item_s, "^1", true)
			|| utils::starts_with(item_s, "ERROR:")
			|| utils::starts_with(item_s, "Error:"))
		{
			color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			has_color = true;

			utils::ltrim(item_s);
		}
		else if (utils::starts_with(item_s, "^2", true))
		{
			color = ImVec4(0.13f, 0.4f, 0.79f, 1.0f);
			has_color = true;

			utils::ltrim(item_s);
		}
		else if (  utils::starts_with(item_s, "[WARN]", true)
				|| utils::starts_with(item_s, "^3WARNING:", true)
				|| utils::starts_with(item_s, "^3", true)
				|| utils::starts_with(item_s, "WARNING:")
				|| utils::starts_with(item_s, "Warning:"))
		{
			color = ImVec4(1.0f, 0.65f, 0.1f, 1.0f);
			has_color = true;

			utils::ltrim(item_s);
		}
		else if (utils::starts_with(item_s, "[!]"))
		{
			color = ImVec4(0.75f, 0.95f, 0.825f, 1.0f);
			has_color = true;
		}

		if (has_color)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, color);
		}

		//ImGui::TextUnformatted(item_s.c_str());

		constexpr auto BUFF_LEN = 256;
		char input[BUFF_LEN];

		{
			const size_t len = strlen(item_s.c_str());
			const size_t bl = (len < BUFF_LEN - 1 ? len : BUFF_LEN - 1);
			((char*)memcpy(input, item_s.c_str(), bl))[bl] = 0;
		}

		ImGui::PushID(index);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ToImVec4(dvars::gui_window_bg_color->current.vector));
		ImGui::PushItemWidth(ImGui::GetWindowSize().x);
		ImGui::InputText("##read_only", input, 256, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopID();
		ImGui::PopStyleColor();
		ImGui::PopItemWidth();

		if (has_color)
		{
			ImGui::PopStyleColor();
		}
	}

	void console_dialog::exec_command(const char* command_line)
	{
		addline("# %s\n", command_line);

		// Insert into history. First find match and delete it so it can be pushed to the back.
		// This isn't trying to be smart or optimal.
		m_history_pos = -1;
		for (int i = m_history.Size - 1; i >= 0; i--)
		{
			if (_stricmp(m_history[i], command_line) == 0)
			{
				free(m_history[i]);
				m_history.erase(m_history.begin() + i);
				break;
			}
		}

		m_history.push_back(_strdup(command_line));

		// Process command
		//game::dvar_s* dvar = nullptr;

		bool was_dvar = false;
		std::string command_s = command_line;
		const auto	space_pos = command_s.find(' ');

		if (space_pos != std::string::npos)
		{
			const std::string dvar_str = command_s.substr(0, space_pos);

			if (const auto dvar = game::Dvar_FindVar(dvar_str.c_str());
				dvar)
			{
				auto value_string = command_s.substr(space_pos);
				utils::trim(value_string);

				game::Dvar_SetFromStringFromSource(value_string.c_str(), dvar, 0);
				was_dvar = true;
			}
		}

		if (!was_dvar && !command_s.empty())
		{
			std::vector<std::string> args;

			if (command_s.find(' ') != std::string::npos)
			{
				args = utils::split(command_s, ' ');
			}
			else
			{
				args.push_back(command_s);
			}

			components::command::execute_command(args);
		}

		// On command input, we scroll to bottom even if m_auto_scroll==false
		m_scroll_to_bottom = true;
	}

	int text_edit_callback_stub(ImGuiInputTextCallbackData* data)
	{
		const auto con = GET_GUI(console_dialog);
		if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
		{
			// on autocomplete tab -> m_should_search_candidates
			if (con->m_should_search_candidates)
			{
				data->EventFlag = ImGuiInputTextFlags_CallbackEdit;
				return con->text_edit_callback(data);
			}
		}

		if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
		{
			if (con->m_autocomplete_candidates.Size == 1)
			{
				con->m_autocomplete_pos = 0;
			}

			if (con->m_autocomplete_pos >= 0)
			{
				const char* autocomplete_str = (con->m_autocomplete_pos >= 0) ? con->m_autocomplete_candidates[con->m_autocomplete_pos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, autocomplete_str);
				data->InsertChars(data->CursorPos, " ");

				con->m_should_search_candidates = true;
			}
		}

		if (data->EventKey == ImGuiKey_UpArrow || data->EventKey == ImGuiKey_DownArrow)
		{
			if (con->m_autocomplete_candidates.Size > 0 && data->CursorPos > 0)
			{
				//const int prev_history_pos = con->m_autocomplete_pos;

				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (con->m_autocomplete_pos - 1 >= -1)
					{
						con->m_autocomplete_pos--;
					}
				}

				if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (con->m_autocomplete_pos + 1 < con->m_autocomplete_candidates.Size)
					{
						con->m_autocomplete_pos++;
					}
				}
			}
			else
			{
				return con->text_edit_callback(data);
			}
		}

		if (con->m_old_input_but_len != data->BufTextLen)
		{
			// reset autocomplete on normal input
			con->m_autocomplete_pos = -1;

			con->m_old_input_but_len = data->BufTextLen;
			return con->text_edit_callback(data);
		}

		return 0;
	}


	int console_dialog::text_edit_callback(ImGuiInputTextCallbackData* data)
	{
		//addline("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackEdit:
		{
			std::string input = data->Buf;
			const auto	space_pos = input.find(' ');

			if (m_should_search_candidates)
			{
				input = input.substr(0, space_pos);
			}

			if (!m_should_search_candidates)
			{
				if (space_pos != std::string::npos)
				{
					if (m_autocomplete_candidates.Size == 1)
					{
						if (const auto dvar = game::Dvar_FindVar(m_autocomplete_candidates[0]);
							dvar)
						{
							m_matched_dvar = dvar;
						}
					}

					break;
				}
			}

			m_autocomplete_candidates.clear();
			m_matched_dvar = nullptr;

			for (auto dvarIter = 0; dvarIter < *game::dvarCount; ++dvarIter)
			{
				// get the dvar from the "sorted" dvar* list
				const auto dvar = reinterpret_cast<game::dvar_s*>(game::sortedDvars[dvarIter]);
				if (!dvar)
				{
					game::printf_to_console("Invalid dvar while trying to autocomplete ...\n");
					break;
				}

				if (_strnicmp(dvar->name, input.c_str(), data->CursorPos) == 0)
				{
					m_autocomplete_candidates.push_back(dvar->name);
				}
			}

			for (auto& cmd : components::command::cmd_names_autocomplete)
			{
				if (utils::starts_with(cmd, input))
				{
					m_autocomplete_candidates.push_back(cmd.c_str());
				}
			}

			if (m_autocomplete_candidates.Size == 1 && m_should_search_candidates)
			{
				if (const auto dvar = game::Dvar_FindVar(m_autocomplete_candidates[0]);
					dvar)
				{
					m_matched_dvar = dvar;
				}
			}

			m_should_search_candidates = false;

			break;
		}

		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = m_history_pos;
			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (m_history_pos == -1) {
					m_history_pos = m_history.Size - 1;
				}
				else if (m_history_pos > 0) {
					m_history_pos--;
				}

			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (m_history_pos != -1)
				{
					if (++m_history_pos >= m_history.Size) {
						m_history_pos = -1;
					}
				}
			}

			// a better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != m_history_pos)
			{
				const char* history_str = (m_history_pos >= 0) ? m_history[m_history_pos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str);

				std::string input = history_str;
				if (input.find(' ') != std::string::npos)
				{
					m_should_search_candidates = true;
				}
			}
		}
		}
		return 0;
	}

	bool console_dialog::gui()
	{
		const auto MIN_WINDOW_SIZE = ImVec2(400.0f, 200.0f);
		const auto INITIAL_WINDOW_SIZE = ImVec2(520, 600);

		ImGui::SetNextWindowSizeConstraints(MIN_WINDOW_SIZE, ImVec2(FLT_MAX, FLT_MAX));
		ImGui::SetNextWindowSize(INITIAL_WINDOW_SIZE, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ggui::get_initial_window_pos(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Console##window", this->get_p_open()))
		{
			ImGui::End();
			return false;
		}

		bool copy_to_clipboard = false;

		// Reserve enough left-over height for 1 separator + 1 input text
		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false/*, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar*/);

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Copy All"))
			{
				copy_to_clipboard = true;
			}

			if (ImGui::Selectable("Clear"))
			{
				clear_log();
			}

			if (ImGui::Selectable("Auto-Scroll", m_auto_scroll))
			{
				m_auto_scroll = m_auto_scroll ? false : true;
			}

			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // Tighten spacing

		if (copy_to_clipboard) 
		{
			ImGui::LogToClipboard();
		}

		if (m_filter.IsActive())
		{
			for (int i = 0; i < m_items.Size; i++)
			{
				const char* item = m_items[i];

				if (!m_filter.PassFilter(item)) 
				{
					continue;
				}

				draw_text_with_color(item, i);
			}
		}
		else
		{
			ImGuiListClipper clipper;
			clipper.Begin(m_items.Size);

			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					draw_text_with_color(m_items[i], i);
				}
			}
			clipper.End();
		}

		ImGui::PopStyleVar(2);

		if (copy_to_clipboard)
		{
			ImGui::LogFinish();
		}

		if (m_scroll_to_bottom || (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		{
			ImGui::SetScrollHereY(1.0f);
		}

		m_scroll_to_bottom = false;

		ImGui::EndChild();

		SPACING(0.0f, 0.1f);

		// Command-line
		const ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
		if (ImGui::InputText("##console_input", m_input_buf, IM_ARRAYSIZE(m_input_buf), input_text_flags, &text_edit_callback_stub, (void*)this))
		{
			char* s = m_input_buf;
			console_dialog::strtrim(s);

			if (s[0])
			{
				exec_command(s);
			}

			m_autocomplete_candidates.clear();
			m_autocomplete_pos = -1;

			strcpy(s, "");
			m_input_reclaim_focus = true;
		}
		m_input_focused = ImGui::IsItemFocused();
		m_post_inputbox_cursor = ImGui::GetCursorScreenPos();

		// Auto-focus on window apparition
		ImGui::SetItemDefaultFocus();

		if (m_input_reclaim_focus)
		{
			m_input_reclaim_focus = false;
			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
		}

		ImGui::SameLine(0, 6.0f);
		const auto pre_filter_pos = ImGui::GetCursorScreenPos();
		m_filter.Draw("##console_filter", ImGui::GetContentRegionAvail().x); //ImGui::GetContentRegionAvailWidth());

		if (!m_filter.IsActive())
		{
			ImGui::SetCursorScreenPos(ImVec2(pre_filter_pos.x + 12.0f, pre_filter_pos.y + 4.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 0.6f));
			ImGui::TextUnformatted("Filter ..");
			ImGui::PopStyleColor();
		}

		static float matchwindow_height = 0;
		ImGui::SetNextWindowPos(m_post_inputbox_cursor);

		if (ImGui::IsWindowDocked())
		{
			RECT _rect;
			GetClientRect(cmainframe::activewnd->GetWindow(), &_rect);
			const int mainframe_height = _rect.bottom - _rect.top;

			if (matchwindow_height > mainframe_height - m_post_inputbox_cursor.y)
			{
				ImGui::SetNextWindowPos(ImVec2(m_post_inputbox_cursor.x, m_post_inputbox_cursor.y - matchwindow_height - 40.0f));
			}
		}

		if (ImGui::IsWindowFocused() && m_autocomplete_candidates.Size != 0 && GET_GUI(console_dialog)->m_old_input_but_len > 0)
		{
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.85f));
			ImGui::Begin("##console_autocomplete", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

			if (m_matched_dvar)
			{
				ImGui::TextUnformatted(m_matched_dvar->name);
				ImGui::SameLine(0, 10.0f);

				std::string dvar_value;
				switch (m_matched_dvar->type)
				{
				case game::dvar_type::boolean:
					dvar_value = "[BOOL]";
					break;

				case game::dvar_type::value:
					dvar_value = "[FLOAT]";
					break;

				case game::dvar_type::vec2:
					dvar_value = "[VEC2]";
					break;

				case game::dvar_type::vec3:
					dvar_value = "[VEC3]";
					break;

				case game::dvar_type::vec4:
					dvar_value = "[VEC4]";
					break;

				case game::dvar_type::integer:
					dvar_value = "[INT]";
					break;

				case game::dvar_type::enumeration:
					dvar_value = "[ENUM]";
					break;

				case game::dvar_type::string:
					dvar_value = " [STRING]";
					break;

				case game::dvar_type::color:
					dvar_value = " [COLOR]";
					break;

				case game::dvar_type::rgb:
					dvar_value = "[RGB]";
					break;

				default:
					dvar_value = "[UNKOWN]";
					break;
				}

				const char* value_to_str = game::Dvar_DisplayableValue(m_matched_dvar);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
				ImGui::TextUnformatted(value_to_str);
				ImGui::PopStyleColor();

				ImGui::SameLine(0, 10.0f);
				ImGui::TextUnformatted(dvar_value.c_str());

				char buf[1028];
				const char* domain_str = game::Dvar_DomainToString_Internal(1024, buf, static_cast<int>(m_matched_dvar->type), 0, m_matched_dvar->domain.value.min, m_matched_dvar->domain.value.max);

				if (domain_str)
				{
					ImGui::TextUnformatted(domain_str);
				}

				if (m_matched_dvar->description)
				{
					ImGui::TextUnformatted(m_matched_dvar->description);
				}
			}
			else
			{
				if (m_autocomplete_candidates.Size <= MAX_MATCHES_TO_SHOW)
				{
					for (auto i = 0; i < m_autocomplete_candidates.Size; i++)
					{
						bool is_highlighted = false;
						if (i == m_autocomplete_pos)
						{
							is_highlighted = true;
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.1f, 1.0f));
						}

						ImGui::TextUnformatted(m_autocomplete_candidates[i]);

						if (is_highlighted) {
							ImGui::PopStyleColor();
						}
					}
				}
				else
				{
					ImGui::Text("Too many matches to draw. [%d] Matches.", m_autocomplete_candidates.Size);
				}
			}

			matchwindow_height = ImGui::GetWindowSize().y;

			ImGui::PopStyleColor();
			ImGui::End(); // autocomplete
		}

		ImGui::End(); // console

		return true;
	}


	// *
	// * 

	// CMainFrame::OnViewConsole
	void console_dialog::on_viewconsole_command()
	{
		const auto& con = GET_GUI(console_dialog);
		if(con->is_inactive_tab() && con->is_active())
		{
			con->set_bring_to_front(true);
			return;
		}

		con->toggle();
	}

	void console_dialog::load_raw_materials_progressbar(int index, int material_total_count)
	{
		const int    idx = index + 1;
		const double percentage = ((double)idx / (double)material_total_count);

		const int val = static_cast<int>(percentage * 100);
		const int lpad = static_cast<int>(percentage * 60);
		const int rpad = 60 - lpad;

		printf("\rLoading raw materials: %3d%% [%.*s%*s] %d/%d", val, lpad, "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||", rpad, "", idx, material_total_count);

		if (val == 100)
		{
			printf("\n");
		}

		fflush(stdout);
	}

	__declspec(naked) void console_dialog::load_raw_materials_progressbar_stub()
	{
		const static uint32_t retn_addr = 0x45AF65;
		__asm
		{
			pushad;
			mov		eax, [ebp - 54h];
			push	eax;
			push	ecx;
			call	console_dialog::load_raw_materials_progressbar;
			add		esp, 8;
			popad;

			add     ecx, 1; // og
			cmp     ecx, [ebp - 54h]; // ebp - 54 = total amount of materials

			jmp		retn_addr;
		}
	}

	void console_dialog::hooks()
	{
		// load raw materials progressbar
		utils::hook::nop(0x45AF5F, 6);
		utils::hook(0x45AF5F, load_raw_materials_progressbar_stub, HOOK_JUMP).install()->quick();

		// NOP startup console-spam
		utils::hook::nop(0x4818DF, 5); // ScanFile
		utils::hook::nop(0x48B8BE, 5); // ScanWeapon

		// silence "Could not connect to source control"
		utils::hook::nop(0x420B59, 5);

		// remove "successful" print when loading models and prefabs -> only print failures
		utils::hook::nop(0x480A96, 5);
		utils::hook::nop(0x480BD1, 5);

		// remove "\n" infront of "\nFile Handles:\n"
		utils::hook::set<BYTE>(0x4A182D + 1, 0x7D);

		// kill Sys_Printf("Updating layers...\n")
		utils::hook::nop(0x45F2A6, 5); // undo undo
		utils::hook::nop(0x48977C, 5); // enter prefab
		utils::hook::nop(0x489BA6, 5); // leave prefab

		// redirect console prints
		utils::hook::nop(0x420A54, 10);
		utils::hook::nop(0x40A9E0, 10);
		utils::hook::set(0x25D5A54, game::printf_to_console_internal); // redirect internal radiant console prints
		utils::hook::detour(0x499E90, game::printf_to_console, HK_JUMP); // sys_printf
		utils::hook::detour(0x40B5D0, game::com_printf_to_console, HK_JUMP); // com_printf
		utils::hook::detour(0x5BE383, game::printf_to_console, HK_JUMP); // printf


		// disable console tab insertion in entitywnd :: CTabCtrl::InsertItem(&g_wndTabsEntWnd, 1u, 2u, "C&onsole", 0, 0);
		utils::hook::nop(0x496713, 23);

		utils::hook::detour(0x496A2B, (void*)0x496AE6, HK_JUMP);
		utils::hook::detour(0x423D2F, (void*)0x423EBC, HK_JUMP);
		utils::hook::detour(0x423E02, (void*)0x423EBC, HK_JUMP);
		utils::hook::detour(0x496B5F, (void*)0x496C68, HK_JUMP);
		utils::hook::detour(0x498457, (void*)0x498ACA, HK_JUMP);

		// make console-view hotkey open the imgui variant :: CMainFrame::OnViewConsole
		utils::hook::detour(0x423CB0, console_dialog::on_viewconsole_command, HK_JUMP);
	}

	console_dialog::~console_dialog()
	{
		clear_log();

		for (int i = 0; i < m_history.Size; i++)
		{
			free(m_history[i]);
		}
	}

	REGISTER_GUI(console_dialog);
}