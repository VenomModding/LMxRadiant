/*
	Copyright 2020 Limeoats

   	Licensed under the Apache License, Version 2.0 (the "License");
   	you may not use this file except in compliance with the License.
   	You may obtain a copy of the License at
	
       	http://www.apache.org/licenses/LICENSE-2.0
	
   	Unless required by applicable law or agreed to in writing, software
   	distributed under the License is distributed on an "AS IS" BASIS,
   	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   	See the License for the specific language governing permissions and
   	limitations under the License.
*/

// based on https://github.com/Limeoats/L2DFileDialog/tree/b902d7faae37a2b1ab9b6b7ce3c123f42963061e

#pragma once

namespace ggui
{
	class file_dialog final : public ggui::ggui_module
	{
	public:
		enum class FileDialogType
		{
			OpenFile,
			SelectFolder,
			SaveFile
		};

	private:
		bool m_update_files_and_folders = false;
		bool m_update_sorting = false;
		bool m_initial_path_set = false;

		int  m_file_select_index = 0;
		int  m_folder_select_index = 0;
		char m_error[500] = {};

		FileDialogType m_file_operation_type = FileDialogType::OpenFile;
		std::string m_file_ext;

		std::string m_default_path;
		std::string m_current_path;
		std::string m_current_file;
		std::string m_current_folder;

		std::string m_save_file_name;
		bool m_save_filename_valid = false;

		// persistent data after dialog was closed
		std::string m_last_filepath;
		int m_last_file_handler = 0;

	public:
		file_dialog()
		{
			set_gui_type(GUI_TYPE_DEF);
		}

		void reset()
		{
			this->m_file_select_index = 0;
			this->m_folder_select_index = 0;
			strcpy_s(this->m_error, "");

			m_file_operation_type = FileDialogType::OpenFile;
			m_file_ext = "";

			set_default_path("");
			this->m_initial_path_set = false;
			this->m_current_path = "";
			this->m_current_file = "";
			this->m_current_folder = "";

			this->close();
		}

		void set_file_op_type(const FileDialogType type)
		{
			if(!this->is_active())
				m_file_operation_type = type;
		}
		[[nodiscard]] FileDialogType get_file_op_type() const
		{
			return m_file_operation_type;
		}

		void set_file_ext(const std::string& ext)
		{
			if (!this->is_active())
				m_file_ext = ext;
		}
		std::string& get_file_ext()
		{
			return m_file_ext;
		}

		void set_default_path(const std::string& path)
		{
			if (!this->is_active())
				m_default_path = path;
		}
		const std::string& get_default_path()
		{
			return m_default_path;
		}

		[[nodiscard]] std::string get_path_result() const
		{
			return m_last_filepath;
		}

		// ggui::FILE_DIALOG_HANDLER
		void set_file_handler(const int handler)
		{
			if (!this->is_active())
				m_last_file_handler = handler;
		}

		// ggui::FILE_DIALOG_HANDLER
		[[nodiscard]] int get_file_handler() const
		{
			return m_last_file_handler;
		}

		bool selection_build_path_to_file()
		{
			if (this->get_file_op_type() == FileDialogType::SelectFolder)
			{
				if (this->m_current_folder.empty())
				{
					strcpy_s(this->m_error, "Error: You must select a folder!");
				}
				else
				{
					m_last_filepath = this->m_current_path + (this->m_current_path.back() == '\\' ? "" : "\\") + this->m_current_folder;
					strcpy_s(this->m_error, "");

					this->reset();
					return true;
				}
			}
			else if (this->get_file_op_type() == FileDialogType::OpenFile)
			{
				if (this->m_current_file.empty())
				{
					strcpy_s(this->m_error, "Error: You must select a file!");
				}
				else
				{
					m_last_filepath = this->m_current_path + (this->m_current_path.back() == '\\' ? "" : "\\") + this->m_current_file;
					strcpy_s(this->m_error, "");

					this->reset();
					return true;
				}
			}
			else if (this->get_file_op_type() == FileDialogType::SaveFile)
			{
				if (this->m_save_file_name.empty())
				{
					strcpy_s(this->m_error, "Error: You must enter a name!");
				}
				else
				{
					m_last_filepath = this->m_current_path + (this->m_current_path.back() == '\\' ? "" : "\\") + this->m_save_file_name;
					strcpy_s(this->m_error, "");

					this->reset();
					return true;
				}
			}

			return false;
		}

		bool dialog();

		void on_open() override;
		void on_close() override;
	};
}