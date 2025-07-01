
#ifdef CSGO_LUA

#include <gui/selectable_script.h>

using namespace evo::gui;
using namespace evo::ren;

void gui::selectable_script::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();

	const auto r = area_abs();

	auto &l = draw.layers[ctx->content_layer];
	l->add_rect_filled(r.width(anim->value.f), colors.accent);

	if (file.metadata.name.has_value())
		text = *file.metadata.name;

	l->font = draw.fonts[is_highlighted ? uint64_t(GUI_HASH("gui_bold")) : uint64_t(GUI_HASH("gui_main"))]; //gui_bald gui_main
	l->add_text(r.tl() + vec2(anim->value.f + 4.f, 2.f), text,
				is_loaded ? colors.accent : anim->value.c);
}

#endif
