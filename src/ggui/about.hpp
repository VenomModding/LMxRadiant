#pragma once

namespace ggui
{
	class about_dialog final : public ggui::ggui_module
	{
	public:
		about_dialog() { set_gui_type(GUI_TYPE_DEF); }
		void gui() override;
	};
}
