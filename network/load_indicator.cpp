#include <network/helpers.h>
#include <network/load_indicator.h>
#include <ren/adapter.h>
#include <ren/renderer.h>

#include <base/cfg.h>
#include <base/game.h>
#include <gui/gui.h>
#include <menu/menu.h>
#include <sdk/surface.h>

#include <sdk/global_vars_base.h>
#include <cmath>

namespace network
{
	using namespace evo::ren;
	using namespace evo::gui;

	load_indicator load_ind;

	void load_indicator::begin()
	{
		alpha = std::make_shared<anim_float>(0.f, 0.5f, ease_linear);
		spinner = std::make_shared<anim_float>(0.f, 1.f, ease_linear);

		alpha->direct(1.f);

		spinner->direct(360.f);
		spinner->on_finished = [&](const std::shared_ptr<anim<float>>& a)
			{
				if (did_begin)
					spinner->direct(0.f, 360.f);
			};

		start_time = game->globals->realtime;
		did_begin = true;
	}

	void load_indicator::end()
	{
		is_ending = true;

		alpha->direct(0.f);
		alpha->on_finished = [&](const std::shared_ptr<anim<float>>& a)
			{
				

				did_begin = false;

				alpha = nullptr;
				spinner = nullptr;

				ctx->reset();
				
			};
		menu::menu.can_toggle = true;
	}

	void load_indicator::render()
	{
		if (!did_begin)
			return;

		auto& d = draw.layers[63];
		d->g.alpha = alpha->value;

		const auto a = rect(0.f, 0.f).size(draw.display);
		add_with_blur(d, a, [&a](auto& d) { d->add_rect_filled(a, color::white()); });
		d->add_rect_filled(a, color(0.f, 0.f, 0.f, 0.7f));

		if (!is_ending)
		{
			const float heartbeat_scale = 1.0f + 0.1f * std::sin(game->globals->realtime * 2.0f * evo::ren::pi);
			const vec2 logo_size = vec2(128.f, 128.f) * heartbeat_scale;
			d->g.texture = ctx->res.general.logo_head->obj;
			d->add_rect_filled(rect(draw.display * 0.5f - logo_size * 0.5f).size(logo_size), color::white().mod_a(alpha->value));
			d->g.texture = {};
			d->g.texture = ctx->res.general.logo_stripes->obj;
			d->add_rect_filled(rect(draw.display * 0.5f - logo_size * 0.5f).size(logo_size), colors.accent.mod_a(alpha->value));
			d->g.texture = {};
		}

		d->g.alpha = {};

		alpha->animate();
		spinner->animate();

		// Check if 5 seconds have passed
		if (game->globals->realtime - start_time >= 5.0f)
		{
			end();
		}

		perform_task();
	}

	void load_indicator::perform_task()
	{
		if (current_section == sec_max)
			return;

		if (current_section == sec_resource_load && !did_finish_section(sec_resource_load))
		{
			//load_required_resources();
			finished_sections.emplace_back(sec_resource_load);
			return;
		}

		if (current_section == sec_script_load && !did_finish_section(sec_script_load))
		{
#ifdef CSGO_LUA
			// refresh available scripts
			const auto refresh = ctx->find<button>(GUI_HASH("scripts.general.refresh"));
			if (refresh)
				refresh->callback();
#endif

			finished_sections.emplace_back(sec_script_load);
			return;
		}

		if (current_section == sec_config_load && !did_finish_section(sec_config_load))
		{
			// refresh available configs
			const auto refresh = ctx->find<button>(GUI_HASH("misc.configs.refresh"));
			if (refresh)
				refresh->callback();

			finished_sections.emplace_back(sec_config_load);
			return;
		}
	}

	void load_indicator::begin_next_section()
	{
		section_progress = 0.f;

		if (current_section + 1 >= sec_max)
		{
			current_section = sec_max;
			end();
			return;
		}

		current_section = (section)((int)current_section + 1);
	}

	bool load_indicator::did_finish_section(section s) const
	{
		return std::find(finished_sections.begin(), finished_sections.end(), s) != finished_sections.end();
	}
} // namespace network

