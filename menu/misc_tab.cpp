
#include <base/cfg.h>
#include <base/game.h>
#include <filesystem>
#include <gui/helpers.h>
#include <gui/popups/about_popup.h>
#include <gui/selectable_config.h>
#include <gui/widgets/active_hotkeys.h>
#include <gui/widgets/bomb.h>
#include <gui/widgets/spectators.h>
#include <menu/macros.h>
#include <menu/menu.h>
#include <sdk/input_system.h>
#include <shellapi.h>
#include <util/fnv1a.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

#if !defined(CSGO_SECURE) || defined(_DEBUG)
#include <base/debug_overlay.h>
#else
#include <network/helpers.h>
#include <network/load_indicator.h>
#endif

USE_NAMESPACES;

void unload();

void menu::group::misc_general(std::shared_ptr<layout>& e)
{
	ADD("bunny hop", "misc.general.bunny_hop", checkbox, cfg.misc.bhop);

	const auto auto_strafe = MAKE("misc.general.auto_strafe", combo_box, cfg.misc.autostrafe);
	auto_strafe->add({
		MAKE("misc.general.auto_strafe.disabled", selectable, XOR("disabled")),
		MAKE("misc.general.auto_strafe.view_direction", selectable, XOR("view direction")),
		MAKE("misc.general.auto_strafe.movement_input", selectable, XOR("movement input")),
		});

	ADD_INLINE("auto strafe", auto_strafe);
	ADD("smoothing factor", "misc.general.auto_strafe_turn_speed", slider<float>, 0.f, 100.f,
		cfg.misc.autostrafe_turn_speed, XOR("%.0f%%"));
	ADD("avoid collision", "misc.general.avoid_collision", checkbox, cfg.misc.avoid_collisions);

	ADD_TOOLTIP("edge jump", "misc.general.edge_jump", "automatically jump before falling of an edge", checkbox,
		cfg.misc.edge_jump);

	ADD_TOOLTIP("nade assistant", "misc.general.nade_assistant",
		"adjusts grenade throw angle to maximize distance while moving", checkbox, cfg.misc.nade_assistant);

	const auto peek_assistant = MAKE("misc.general.peek_assistant_mode", combo_box, cfg.misc.peek_assistant_mode);
	peek_assistant->add({ MAKE("misc.general.peek_assistant_mode.free_roam", selectable, XOR("free roaming")),
						 MAKE("misc.general.peek_assistant_mode.retreat_early", selectable, XOR("retreat early")) });
	ADD_INLINE("peek assistant", MAKE("misc.general.peek_assistant", checkbox, cfg.misc.peek_assistant));
	ADD_LINE("misc.general.peek_assistant", peek_assistant,
		MAKE("misc.general.peek_assistant_color", color_picker, cfg.misc.peek_assistant_color));

	ADD("reveal ranks", "misc.general.reveal_ranks", checkbox, cfg.misc.reveal_ranks);
	ADD("auto accept", "misc.general.auto_accept", checkbox, cfg.misc.auto_accept);
	ADD("clantag changer", "misc.general.clantag_changer", checkbox, cfg.misc.clantag_changer);
	ADD("preserve killfeed", "misc.general.preserve_killfeed", checkbox, cfg.misc.preserve_killfeed);
	ADD("unlock inventory", "misc.general.unlock_inventory", checkbox, cfg.misc.unlock_inventory);

	const auto auto_buy = MAKE("misc.general.auto_buy", checkbox, cfg.misc.bb_enable);
	auto_buy->aliases = { XOR_STR("buybot"), XOR_STR("buy"), XOR_STR("bot") };
	ADD_INLINE("auto buy", auto_buy);

	const auto bb_primary = MAKE("misc.general.auto_buy.primary", combo_box, cfg.misc.bb_primary);
	bb_primary->add({
		MAKE("misc.general.auto_buy.primary.none", selectable, XOR("none")),
		MAKE("misc.general.auto_buy.primary.scout", selectable, XOR("ssg-08")),
		MAKE("misc.general.auto_buy.primary.auto_sniper", selectable, XOR("g3sg1 / scar-20")),
		MAKE("misc.general.auto_buy.primary.awp", selectable, XOR("awp")),
		});

	const auto bb_secondary = MAKE("misc.general.auto_buy.secondary", combo_box, cfg.misc.bb_secondary);
	bb_secondary->add({
		MAKE("misc.general.auto_buy.secondary.none", selectable, XOR("none")),
		MAKE("misc.general.auto_buy.secondary.duals", selectable, XOR("dual berettas")),
		MAKE("misc.general.auto_buy.secondary.tec", selectable, XOR("tec-9 / five-seven / cz75-auto")),
		MAKE("misc.general.auto_buy.secondary.deagle", selectable, XOR("r8 revolver / desert eagle")),
		});

	const auto bb_utilities = MAKE("misc.general.auto_buy.utilities", combo_box, cfg.misc.bb_utilities);
	bb_utilities->allow_multiple = true;
	bb_utilities->add({
		MAKE("misc.general.auto_buy.utilities.smoke", selectable, XOR("smoke")),
		MAKE("misc.general.auto_buy.utilities.he", selectable, XOR("high explosive")),
		MAKE("misc.general.auto_buy.utilities.flash", selectable, XOR("flashbang")),
		MAKE("misc.general.auto_buy.utilities.flash2", selectable, XOR("flashbang x2")),
		MAKE("misc.general.auto_buy.utilities.molotov", selectable, XOR("molotov / incendiary")),
		MAKE("misc.general.auto_buy.utilities.taser", selectable, XOR("zeus x27")),
		MAKE("misc.general.auto_buy.utilities.defuse", selectable, XOR("defuse kit")),
		MAKE("misc.general.auto_buy.utilities.armor", selectable, XOR("armor")),
		MAKE("misc.general.auto_buy.utilities.helmet", selectable, XOR("armor + helmet")),
		});

	auto_buy->callback = [](bool v)
		{
			const auto bb_primary = ctx->find(GUI_HASH("misc.general.auto_buy.primary"));
			const auto bb_secondary = ctx->find(GUI_HASH("misc.general.auto_buy.secondary"));
			const auto bb_utilities = ctx->find(GUI_HASH("misc.general.auto_buy.utilities"));

			if (!bb_primary || !bb_secondary || !bb_utilities)
				return;

			bb_primary->parent.lock()->set_visible(v);
			bb_secondary->parent.lock()->set_visible(v);
			bb_utilities->parent.lock()->set_visible(v);
		};

	ADD_INLINE("primary", bb_primary);
	ADD_INLINE("secondary", bb_secondary);
	ADD_INLINE("utilities", bb_utilities);

#ifdef _DEBUG
	const auto btn = MAKE("misc.general.on_exit", button, XOR("unload"));
	btn->callback = unload;

	e->add(btn);
#endif
}


void menu::group::misc_settings(std::shared_ptr<layout>& e)
{
	const auto menu_color = MAKE("misc.settings.menu_color", color_picker, cfg.misc.menu_color, false);
	menu_color->callback = [](color c) { colors.accent = c; };

	ADD_INLINE("menu color", menu_color);
	ADD("enable menu sounds", "misc.settings.enable_tick_sound", checkbox, cfg.misc.enable_tick_sound);

	const auto menu_movement = MAKE("misc.settings.enable_menu_movement", checkbox, cfg.misc.menu_movement);
	menu_movement->callback = [](bool b) { game->input_system->reset_input_state(); };

	ADD_INLINE("enable menu movement", menu_movement);

	const auto blur_quality = MAKE("misc.settings.blur_quality", combo_box, ctx->blur_quality);
	blur_quality->add({
		MAKE("misc.settings.blur_quality.quality", selectable, XOR("quality")),
		MAKE("misc.settings.blur_quality.performance", selectable, XOR("performance")),
		MAKE("misc.settings.blur_quality.disabled", selectable, XOR("disabled")),
		});

	ADD_INLINE("blur effect", blur_quality);
}


void menu::group::misc_helpers(std::shared_ptr<layout>& e)
{
	const auto event_log_triggers = MAKE("misc.helpers.event_log_triggers", combo_box, cfg.misc.event_triggers);
	event_log_triggers->allow_multiple = true;
	event_log_triggers->add({
		MAKE("misc.helpers.event_log_triggers.shot_info", selectable, XOR("shot info")),
		MAKE("misc.helpers.event_log_triggers.damage_taken", selectable, XOR("damage taken")),
		MAKE("misc.helpers.event_log_triggers.purchase", selectable, XOR("purchase")),
		});

	ADD_TOOLTIP("event log", "misc.helpers.event_log_output",
		"prints the data of your events", checkbox, cfg.misc.eventlog);

	ADD_INLINE("event log triggers", event_log_triggers);
	
	

	const auto indicators = MAKE("misc.helpers.indicators", combo_box, cfg.indicators.indicators);
	indicators->allow_multiple = true;
	indicators->add({
		MAKE("misc.helpers.indicators.hotkeys", selectable, XOR("active hotkeys")),
		MAKE("misc.helpers.indicators.spectators", selectable, XOR("spectators")),
		MAKE("misc.helpers.indicators.bomb", selectable, XOR("bomb")),
		});

	indicators->callback = [](bits& v)
		{
			const auto hotkeys = ctx->find_all(::gui::widgets::active_hotkeys_id);
			const auto spectators = ctx->find_all(::gui::widgets::spectators_id);
			const auto bomb = ctx->find_all(::gui::widgets::bomb_id);

			// dumb fix for dumb issue
			if (hotkeys.size() > 1)
			{
				for (auto i = 1; i < hotkeys.size(); i++)
					ctx->remove(hotkeys[i]);
			}

			if (spectators.size() > 1)
			{
				for (auto i = 1; i < spectators.size(); i++)
					ctx->remove(spectators[i]);
			}

			if (bomb.size() > 1)
			{
				for (auto i = 1; i < bomb.size(); i++)
					ctx->remove(bomb[i]);
			}

			const auto ui_vis = menu.main_wnd.lock() && menu.main_wnd.lock()->is_visible;
			if (v.test(cfg_t::ui_indicators_hotkeys) && hotkeys.empty())
			{
				const auto wdg = std::make_shared<::gui::widgets::active_hotkeys>(cfg.indicators.active_hotkeys_pos);
				wdg->is_forced_visibility = ui_vis;
				wdg->is_taking_input = ui_vis;
				wdg->toggle_visibility(ui_vis);

				ctx->add(wdg);
			}
			else if (!v.test(cfg_t::ui_indicators_hotkeys) && !hotkeys.empty())
				ctx->remove(hotkeys.back());

			if (v.test(cfg_t::ui_indicators_spectators) && spectators.empty())
			{
				const auto wdg = std::make_shared<::gui::widgets::spectators>(cfg.indicators.spectators_pos);
				wdg->is_forced_visibility = ui_vis;
				wdg->is_taking_input = ui_vis;
				wdg->toggle_visibility(ui_vis);

				ctx->add(wdg);
			}
			else if (!v.test(cfg_t::ui_indicators_spectators) && !spectators.empty())
				ctx->remove(spectators.back());

			if (v.test(cfg_t::ui_indicators_bomb) && bomb.empty())
			{
				const auto wdg = std::make_shared<::gui::widgets::bomb>(cfg.indicators.bomb_pos);
				wdg->is_forced_visibility = ui_vis;
				wdg->is_taking_input = ui_vis;
				wdg->toggle_visibility(ui_vis);

				ctx->add(wdg);
			}
			else if (!v.test(cfg_t::ui_indicators_bomb) && !bomb.empty())
				ctx->remove(bomb.back());
		};

	ADD_INLINE("indicators", indicators);
	ADD("alpha override", "misc.helpers.alpha_override", slider<float>, 0.f, 100.f, cfg.indicators.alpha,
		XOR_STR("%.0f%%"));
}

void menu::group::misc_configs(std::shared_ptr<layout> &e)
{
	static value_param<bits> selected{};
	static std::mutex refresh_mtx{};

	const auto cfg_refresh = MAKE("misc.configs.refresh", button, XOR(""), draw.textures[GUI_HASH("icon_refresh")]);
	cfg_refresh->render_bg = true;
	cfg_refresh->adjust_margin({0.f, 0.f, 0.f, 8.f});
	cfg_refresh->size = {30.f, 30.f};
	cfg_refresh->icon_size = {16.f, 16.f};
	cfg_refresh->tooltip = XOR("refresh");
	cfg_refresh->callback = []()
	{
		if (cfg.is_loading)
			return;

		std::thread(
			[]()
			{
				static bool first_time_load{};
				if (!refresh_mtx.try_lock())
					return;

				cfg.is_updating = true;

				const auto l = ctx->find<list>(GUI_HASH("misc.configs.list"));
				if (!l)
					return;

				l->clear();
				l->show_spinner = true;

#if defined(CSGO_SECURE) && !defined(_DEBUG)
				network::refresh_configs();
#endif

				int i{};
				for (const auto &f : cfg.remote_configs)
				{
					const auto sel = std::make_shared<::gui::selectable_config>(
						evo::gui::control_id{GUI_HASH("remote_config") + f.id}, f.name, f.id);
					if (cfg.last_changed_config == tfm::format(XOR("remote_%i"), f.id))
					{
						sel->is_loaded = true;
						selected = i;
						sel->select();
					}

					l->add(sel);
					++i;
				}

				// get file count
				int total_files{};
				for (const auto &p : std::filesystem::directory_iterator(XOR_STR("ev0lve")))
					if (p.path().extension() == XOR_STR(".cfg"))
						++total_files;

				const auto progress_fraction = (1.f / (float)total_files) * 0.5f;
				for (const auto &f : std::filesystem::directory_iterator(XOR("ev0lve")))
				{
					const auto &path = f.path();
					if (path.extension() != XOR(".cfg"))
						continue;

					const auto sel = std::make_shared<::gui::selectable_config>(
						evo::gui::control_id{evo::gui::hash(path.string().c_str())},
						path.filename().replace_extension("").string());

					if (cfg.last_changed_config == XOR("ev0lve/") + sel->text + XOR(".cfg") ||
						sel->text == XOR("default") && !first_time_load)
					{
						sel->is_loaded = true;
						selected = i;
						sel->select();
					}

					l->add(sel);
					++i;

#if defined(CSGO_SECURE) && !defined(_DEBUG)
					network::load_ind.section_progress += progress_fraction;
#endif
				}

				l->show_spinner = false;
				l->process_queues();

#if defined(CSGO_SECURE) && !defined(_DEBUG)
				if (network::load_ind.did_begin && !network::load_ind.is_ending)
				{
					network::load_ind.begin_next_section();
					cfg.load(XOR_STR("ev0lve/default.cfg"), true);
				}
#else
				if (!first_time_load)
					cfg.load(XOR_STR("ev0lve/default.cfg"), true);
#endif

				first_time_load = true;
				cfg.is_updating = false;
				refresh_mtx.unlock();
			})
			.detach();
	};

	const auto cfg_load = MAKE("misc.configs.load", button, XOR(""), draw.textures[GUI_HASH("icon_load")]);
	cfg_load->render_bg = true;
	cfg_load->adjust_margin({});
	cfg_load->size = {30.f, 30.f};
	cfg_load->icon_size = {16.f, 16.f};
	cfg_load->tooltip = XOR("load");
	cfg_load->callback = []()
	{
		if (cfg.is_loading)
			return;

		const auto cfg_list = ctx->find<list>(GUI_HASH("misc.configs.list"));

		if (!cfg_list)
			return;

		const auto s = cfg_list->at(selected.get().first_set_bit().value_or(0))->as<::gui::selectable_config>();
		if (!s)
			return;

		if (s->remote_id > 0)
		{
			const auto rc = std::find_if(cfg.remote_configs.begin(), cfg.remote_configs.end(),
										 [s](const auto &f) { return f.id == s->remote_id; });

			if (rc == cfg.remote_configs.end())
				return;

			cfg.load_from_string(rc->content);
			cfg.last_changed_config = tfm::format(XOR("remote_%i"), rc->id);
		}
		else
			cfg.load(XOR("ev0lve/") + s->text + XOR(".cfg"));

		cfg_list->for_each_control(
			[&](std::shared_ptr<control> &c)
			{
				const auto sel = c->as<::gui::selectable_config>();
				if (sel->remote_id <= 0)
				{
					if (cfg.last_changed_config == XOR("ev0lve/") + sel->text + XOR(".cfg"))
						sel->is_loaded = true;
					else
						sel->is_loaded = false;
				}
				else
				{
					if (cfg.last_changed_config == tfm::format(XOR("remote_%i"), sel->remote_id))
						sel->is_loaded = true;
					else
						sel->is_loaded = false;
				}

				sel->reset();
			});
	};

	const auto cfg_save = MAKE("misc.configs.save", button, XOR(""), draw.textures[GUI_HASH("icon_save")]);
	cfg_save->render_bg = true;
	cfg_save->adjust_margin({});
	cfg_save->size = {30.f, 30.f};
	cfg_save->icon_size = {16.f, 16.f};
	cfg_save->tooltip = XOR("save");
	cfg_save->callback = []()
	{
		if (cfg.is_loading)
			return;

		const auto cfg_list = ctx->find<list>(GUI_HASH("misc.configs.list"));
		if (!cfg_list)
			return;
		const auto s = cfg_list->at(selected.get().first_set_bit().value_or(0))->as<selectable>();
		if (!s)
			return;

		const auto file = XOR("ev0lve/") + s->text + XOR(".cfg");

		// open dialog if we are unsure
		if (cfg.is_dangerous || cfg.last_changed_config != file)
		{
			cfg.want_to_change = file;

			const auto dlg =
				std::make_shared<dialog_box>(evo::gui::control_id{GUI_HASH("config.save")}, XOR("Warning"),
											 XOR("Are you sure you want to overwrite this config?"), db_yes_no);
			dlg->callback = [](dialog_result r)
			{
				if (r == dr_affirmative)
					cfg.save(cfg.want_to_change);
			};

			dlg->open();
		}
		else
			cfg.save(file);

		cfg_list->for_each_control(
			[&](std::shared_ptr<control> &c)
			{
				const auto sel = c->as<selectable>();
				if (file == XOR("ev0lve/") + sel->text + XOR(".cfg"))
					sel->is_loaded = true;
				else
					sel->is_loaded = false;

				sel->reset();
			});
	};

	const auto cfg_reset = MAKE("misc.configs.reset", button, XOR(""), draw.textures[GUI_HASH("icon_reset")]);
	cfg_reset->render_bg = true;
	cfg_reset->adjust_margin({});
	cfg_reset->size = {30.f, 30.f};
	cfg_reset->icon_size = {16.f, 16.f};
	cfg_reset->tooltip = XOR("reset");
	cfg_reset->callback = []()
	{
		if (cfg.is_loading)
			return;

		cfg.drop();
	};

	const auto cfg_create = MAKE("misc.configs.create", button, XOR(""), ctx->res.icons.add);
	cfg_create->render_bg = true;
	cfg_create->adjust_margin({});
	cfg_create->size = {30.f, 30.f};
	cfg_create->icon_size = {16.f, 16.f};
	cfg_create->tooltip = XOR("create");
	cfg_create->callback = []()
	{
		if (cfg.is_loading)
			return;

		const auto inp = ctx->find<text_input>(GUI_HASH("misc.configs.name"));
		const auto refresh = ctx->find<button>(GUI_HASH("misc.configs.refresh"));

		if (!inp || !refresh)
			return;

		// trim the text
		auto name = inp->value;
		name.erase(std::remove_if(name.begin(), name.end(), [](const auto &c) { return c == ' '; }), name.end());

		if (name.empty())
			return;

		std::ofstream _file(XOR("ev0lve/") + inp->value + XOR(".cfg"));
		_file.close();

		refresh->callback();
	};

	const auto cfg_delete = MAKE("misc.configs.delete", button, XOR(""), ctx->res.icons.remove);
	cfg_delete->render_bg = true;
	cfg_delete->adjust_margin({0.f, 8.f, 0.f, 0.f});
	cfg_delete->size = {30.f, 30.f};
	cfg_delete->icon_size = {16.f, 16.f};
	cfg_delete->tooltip = XOR("delete");
	cfg_delete->color = colors.danger;
	cfg_delete->callback = []()
	{
		if (cfg.is_loading)
			return;

		const auto s = ctx->find<list>(GUI_HASH("misc.configs.list"))
						   ->at(selected.get().first_set_bit().value_or(0))
						   ->as<selectable>();
		if (!s)
			return;

		const auto file = XOR("ev0lve/") + s->text + XOR(".cfg");

		cfg.want_to_change = file;

		const auto dlg = std::make_shared<dialog_box>(evo::gui::control_id{GUI_HASH("config.delete")}, XOR("Warning"),
													  XOR("Are you sure you want to delete this config?"), db_yes_no);
		dlg->callback = [](dialog_result r)
		{
			if (r == dr_affirmative)
			{
				std::filesystem::remove(cfg.want_to_change);
				const auto refresh = ctx->find<button>(GUI_HASH("misc.configs.refresh"));
				if (refresh)
					refresh->callback();
			}
		};

		dlg->open();
	};

	const auto cfg_open_folder =
		MAKE("misc.configs.open_folder", button, XOR(""), draw.textures[GUI_HASH("icon_folder")]);
	cfg_open_folder->render_bg = true;
	cfg_open_folder->adjust_margin({});
	cfg_open_folder->size = {30.f, 30.f};
	cfg_open_folder->icon_size = {16.f, 16.f};
	cfg_open_folder->tooltip = XOR("open Folder");
	cfg_open_folder->callback = []()
	{
		std::thread(
			[&]
			{
				char path[MAX_PATH];
				GetCurrentDirectoryA(MAX_PATH, path);

				ShellExecuteA(nullptr, nullptr, (std::string(path) + XOR("/ev0lve/")).c_str(), nullptr, nullptr,
							  SW_SHOWNORMAL);
			})
			.detach();
	};

	const auto cfg_export = MAKE("misc.configs.export", button, XOR(""), draw.textures[GUI_HASH("icon_export")]);
	cfg_export->render_bg = true;
	cfg_export->adjust_margin({});
	cfg_export->size = {30.f, 30.f};
	cfg_export->icon_size = {16.f, 16.f};
	cfg_export->tooltip = XOR("export to clipboard");
	cfg_export->callback = []() { cfg.export_to_clipboard(); };

	const auto cfg_import = MAKE("misc.configs.import", button, XOR(""), draw.textures[GUI_HASH("icon_import")]);
	cfg_import->render_bg = true;
	cfg_import->adjust_margin({0.f, 8.f, 0.f, 8.f});
	cfg_import->size = {30.f, 30.f};
	cfg_import->icon_size = {16.f, 16.f};
	cfg_import->tooltip = XOR("import from clipboard");
	cfg_import->callback = []() { cfg.import_from_clipboard(); };

	const auto cfg_list = std::make_shared<list>(evo::gui::control_id{GUI_HASH("misc.configs.list")}, selected, vec2(),
												 vec2(174.f, 120.f));
	cfg_list->size_to_parent_h = true;
	cfg_list->callback = [](bits &s)
	{
		if (!ctx->find<list>(GUI_HASH("misc.configs.list")))
			return;

		const auto sel = ctx->find<list>(GUI_HASH("misc.configs.list"))
							 ->at(s.first_set_bit().value_or(0))
							 ->as<::gui::selectable_config>();

		const auto save = ctx->find(GUI_HASH("misc.configs.save"));
		const auto del = ctx->find(GUI_HASH("misc.configs.delete"));
		const auto space = ctx->find(GUI_HASH("misc.configs.spacer"));

		if (!save || !del || !space)
			return;

		if (sel->remote_id <= 0)
		{
			space->size.y = 38.f;
			save->set_visible(true);
			del->set_visible(true);
		}
		else
		{
			space->size.y = 110.f;
			save->set_visible(false);
			del->set_visible(false);
		}
	};

	const auto cfg_name_area = MAKE("misc.configs.name_area", layout, vec2(), vec2(215.f, 30.f));
	cfg_name_area->direction = s_inline;
	cfg_name_area->adjust_margin({2.f, 2.f, 2.f, 10.f});
	cfg_name_area->add(MAKE("misc.configs.name", text_input, vec2{}, vec2{174.f, 30.f})
						   ->make_placeholder(XOR("config name"))
						   ->adjust_margin({}));
	cfg_name_area->add(cfg_create);

	const auto cfg_control_area = MAKE("misc.configs.control_area", layout, vec2(), vec2(30.f, 120.f));
	cfg_control_area->size_to_parent_h = true;
	cfg_control_area->direction = s_vertical;
	cfg_control_area->add(cfg_refresh);
	cfg_control_area->add(cfg_open_folder);
	cfg_control_area->add(MAKE("misc.configs.spacer1", spacer, vec2(), vec2(30.f, 20.f)));
	cfg_control_area->add(cfg_load);
	cfg_control_area->add(cfg_save);
	cfg_control_area->add(cfg_import);
	cfg_control_area->add(cfg_export);
	cfg_control_area->add(MAKE("misc.configs.spacer", spacer, vec2(), vec2(30.f, 38.f)));
	cfg_control_area->add(cfg_reset);
	cfg_control_area->add(cfg_delete);

	const auto cfg_list_area = MAKE("misc.configs.list_area", layout, vec2(), vec2(215.f, 340.f));
	cfg_list_area->direction = s_inline;
	cfg_list_area->adjust_margin({2.f, 2.f, 2.f, 2.f});
	cfg_list_area->add(cfg_list);
	cfg_list_area->add(cfg_control_area);

	e->add(cfg_name_area);
	e->add(cfg_list_area);
}

void unload()
{
#ifndef NDEBUG
	std::thread(
		[]()
		{
			game->deinit();
			game = nullptr;
		})
		.detach();
#endif
}