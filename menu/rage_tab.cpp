
#include <base/cfg.h>
#include <menu/macros.h>
#include <menu/menu.h>

#include <gui/helpers.h>

USE_NAMESPACES;

void menu::group::rage_tab(std::shared_ptr<layout>& e)
{
	const auto side_bar = MAKE("rage.sidebar", tabs_layout, vec2{ -10.f, -10.f }, vec2{ 200.f, 440.f }, td_vertical, true);
	side_bar->add(
		MAKE("rage.sidebar.general", child_tab, XOR("general"), GUI_HASH("rage.weapon_tab.general"))->make_selected());
	side_bar->add(MAKE("rage.sidebar.pistol", child_tab, XOR("pistols"), GUI_HASH("rage.weapon_tab.pistol")));
	side_bar->add(
		MAKE("rage.sidebar.heavy_pistol", child_tab, XOR("heavy pistols"), GUI_HASH("rage.weapon_tab.heavy_pistol")));
	side_bar->add(MAKE("rage.sidebar.automatic", child_tab, XOR("automatic"), GUI_HASH("rage.weapon_tab.automatic")));
	side_bar->add(MAKE("rage.sidebar.awp", child_tab, XOR("awp"), GUI_HASH("rage.weapon_tab.awp")));
	side_bar->add(MAKE("rage.sidebar.scout", child_tab, XOR("ssg-08"), GUI_HASH("rage.weapon_tab.scout")));
	side_bar->add(
		MAKE("rage.sidebar.auto_sniper", child_tab, XOR("auto snipers"), GUI_HASH("rage.weapon_tab.auto_sniper")));

	side_bar->process_queues();
	side_bar->for_each_control([](std::shared_ptr<control>& c) { c->as<child_tab>()->make_vertical(); });

	const vec2 group_sz{ 284.f, 430.f };
	const vec2 group_half_sz = group_sz * vec2{ 1.f, 0.5f } - vec2{ 0.f, 5.f };
	const vec2 group_third_sz = group_sz * vec2{ 1.f, 0.33f } - vec2{ 0.f, 6.5f };

	const auto weapon_tabs = MAKE("rage.weapon_tabs", layout, vec2{}, group_half_sz, s_justify);
	weapon_tabs->make_not_clip();
	weapon_tabs->add(rage_weapon(XOR("general"), grp_general, group_half_sz, true));
	weapon_tabs->add(rage_weapon(XOR("pistol"), grp_pistol, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("heavy_pistol"), grp_heavypistol, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("automatic"), grp_automatic, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("awp"), grp_awp, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("scout"), grp_scout, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("auto_sniper"), grp_autosniper, group_half_sz));

	const auto groups = std::make_shared<layout>(evo::gui::control_id{ GUI_HASH("rage.groups"), XOR_STR("rage.groups") },
		vec2{ 200.f, 0.f }, vec2{ 580.f, 430.f }, s_justify);
	groups->add(tools::make_stacked_groupboxes(
		GUI_HASH("rage.groups.general_weapon"), group_sz,
		{ make_groupbox(XOR("rage.general"), XOR("general"), group_half_sz, rage_general), weapon_tabs }));

	groups->add(tools::make_stacked_groupboxes(
		GUI_HASH("rage.groups.antiaim_fakelag_desync"), group_sz,
		{ make_groupbox(XOR("rage.antiaim"), XOR("anti-aim"), group_third_sz, rage_antiaim),
		 make_groupbox(XOR("rage.desync"), XOR("desync"), group_third_sz, rage_desync),
		 make_groupbox(XOR("rage.fakelag"), XOR("fakelag"), group_third_sz, rage_fakelag) }));

	e->add(side_bar);
	e->add(groups);
}

void menu::group::rage_general(std::shared_ptr<layout>& e)
{
	const auto mode = MAKE("rage.general.mode", combo_box, cfg.rage.mode);
	mode->add({
		MAKE("rage.general.mode.just_in_time", selectable, XOR("just in time"))
			->make_tooltip(XOR("regular ragebot mode")),
		MAKE("rage.general.mode.ahead_of_time", selectable, XOR("ahead of time"))
			->make_tooltip(XOR("increases chance of hitting a running player but disables backtracking")),
		});

	ADD_INLINE("mode", mode, MAKE("rage.general.enable", checkbox, cfg.rage.enable));
	ADD("fov", "rage.general.fov", slider<float>, 0.f, 180.f, cfg.rage.fov, XOR("%.0f°"));

	const std::vector<std::string> only_visible_aliases{ XOR("autowall"), XOR("auto wall") };
	ADD_TOOLTIP_ALIASES("target visible only", "rage.general.only_visible",
		"disables autowall, shoots only visible enemies", checkbox, only_visible_aliases,
		cfg.rage.only_visible);

	const std::vector<std::string> auto_engage_aliases{ XOR("autofire"), XOR("auto fire"), XOR("autostop"),
													   XOR("auto stop") };
	ADD_TOOLTIP_ALIASES("auto engage", "rage.general.auto_engage", "enables automatic fire and stop", checkbox,
		auto_engage_aliases, cfg.rage.auto_engage);
	ADD("target dormant", "rage.general.dormant", checkbox, cfg.rage.dormant);

	const auto fast_fire_mode = MAKE("rage.general.fast_fire_mode", combo_box, cfg.rage.fast_fire);
	fast_fire_mode->add({
		MAKE("rage.general.fast_fire_mode.default", selectable, XOR("default"))
			->make_tooltip(XOR("default (offensive) fast fire mode")),
		MAKE("rage.general.fast_fire_mode.peek", selectable, XOR("peek"))
			->make_tooltip(XOR("peeking (defensive) fast fire mode")),
		});

	const std::vector<std::string> fast_fire_aliases{ XOR("doubletap"), XOR("double tap"), XOR("rapidfire"),
													 XOR("dt") };
	ADD_INLINE_ALIASES("double tap", fast_fire_aliases, fast_fire_mode,
		MAKE("rage.general.fast_fire", checkbox, cfg.rage.enable_fast_fire));
	ADD("hide shots", "rage.general.hide_shot", checkbox, cfg.rage.hide_shot);
	ADD("fake duck", "rage.general.fake_duck", checkbox, cfg.rage.fake_duck);
	ADD("damage override", "rage.general.damage_override", checkbox, cfg.rage.damage_override);
	ADD_INLINE("fake ping",
		MAKE("rage.general.fake_ping", slider<float>, 0.f, 1000.f, cfg.rage.fake_ping_amount, XOR("%.0f ms")),
		MAKE("rage.general.fake_ping_enable", checkbox, cfg.rage.fake_ping));

	const auto slow_walk = MAKE("rage.general.slow_walk", combo_box, cfg.rage.slowwalk_mode);
	slow_walk->add({
		MAKE("rage.general.slow_walk.optimal", selectable, XOR("prefer optimal")),
		MAKE("rage.general.slow_walk.prefer_speed", selectable, XOR("prefer speed")),
		});

	ADD_INLINE("slow walk", slow_walk, MAKE("rage.general.slow_walk_enable", checkbox, cfg.rage.slowwalk));
}


void menu::group::rage_antiaim(std::shared_ptr<layout>& e)
{
	const auto disablers = MAKE("rage.antiaim.disablers", combo_box, cfg.antiaim.disablers);
	disablers->add({ MAKE("rage.antiaim.disablers.round_end", selectable, XOR("disable at round end")),
					MAKE("rage.antiaim.disablers.knife", selectable, XOR("disable against knives")),
					MAKE("rage.antiaim.disablers.defuse", selectable, XOR("disable when defusing")),
					MAKE("rage.antiaim.disablers.shield", selectable, XOR("disable with riot shield")) });
	disablers->allow_multiple = true;
	ADD_INLINE("mode", disablers, MAKE("rage.antiaim.enable", checkbox, cfg.antiaim.enable));

	const auto pitch = MAKE("rage.antiaim.pitch", combo_box, cfg.antiaim.pitch);
	pitch->add({ MAKE("rage.antiaim.pitch.none", selectable, XOR("none")),
				MAKE("rage.antiaim.pitch.down", selectable, XOR("down")),
				MAKE("rage.antiaim.pitch.up", selectable, XOR("up")) });

	ADD_INLINE("pitch", pitch);

	const auto yaw = MAKE("rage.antiaim.yaw", combo_box, cfg.antiaim.yaw);
	yaw->add({ MAKE("rage.antiaim.yaw.view_angle", selectable, XOR("view angle")),
			  MAKE("rage.antiaim.yaw.at_target", selectable, XOR("at target")),
			  MAKE("rage.antiaim.yaw.freestanding", selectable, XOR("freestanding")) });

	ADD_INLINE("yaw", yaw);

	const auto yaw_override = MAKE("rage.antiaim.yaw_override", combo_box, cfg.antiaim.yaw_override);
	yaw_override->add({
		MAKE("rage.antiaim.yaw_override.none", selectable, XOR("none")),
		MAKE("rage.antiaim.yaw_override.left", selectable, XOR("left")),
		MAKE("rage.antiaim.yaw_override.right", selectable, XOR("right")),
		MAKE("rage.antiaim.yaw_override.back", selectable, XOR("back")),
		MAKE("rage.antiaim.yaw_override.forward", selectable, XOR("forward")),
		});

	ADD_INLINE("yaw override", yaw_override);

	ADD("yaw offset", "rage.antiaim.yaw_offset", slider<float>, -180.f, 180.f, cfg.antiaim.yaw_offset, XOR("%.0f°"));

	const auto yaw_modifier = MAKE("rage.antiaim.yaw_modifier", combo_box, cfg.antiaim.yaw_modifier);
	yaw_modifier->add({ MAKE("rage.antiaim.yaw_modifier.none", selectable, XOR("none")),
					   MAKE("rage.antiaim.yaw_modifier.spin", selectable, XOR("spin")),
					   MAKE("rage.antiaim.yaw_modifier.jitter", selectable, XOR("jitter")) });

	ADD_INLINE("yaw modifier", yaw_modifier);

	ADD("yaw modifier amount", "rage.antiaim.yaw_modifier_amount", slider<float>, -180.f, 180.f,
		cfg.antiaim.yaw_modifier_amount, XOR("%.0f°"));

	const auto movement_mode = MAKE("rage.antiaim.movement_mode", combo_box, cfg.antiaim.movement_mode);
	movement_mode->add({
		MAKE("rage.antiaim.movement_mode.default", selectable, XOR("default")),
		MAKE("rage.antiaim.movement_mode.skate", selectable, XOR("skate")),
		MAKE("rage.antiaim.movement_mode.walk", selectable, XOR("walk")),
		MAKE("rage.antiaim.movement_mode.opposite", selectable, XOR("opposite")),
		});

	ADD_INLINE("movement mode", movement_mode);
}


void menu::group::rage_fakelag(std::shared_ptr<layout>& e)
{
	const auto mode = MAKE("rage.fakelag.mode", combo_box, cfg.fakelag.mode);
	mode->add({ MAKE("rage.fakelag.mode.none", selectable, XOR("none")),
			   MAKE("rage.fakelag.mode.dynamic", selectable, XOR("dynamic")),
			   MAKE("rage.fakelag.mode.maximum", selectable, XOR("maximum")) });
	ADD_INLINE("mode", mode);
	ADD("amount", "rage.fakelag.amount", slider<float>, 0.f, 14.f, cfg.fakelag.lag_amount, XOR("%.0f ticks"));
	ADD("variance", "rage.fakelag.variance", slider<float>, 0.f, 14.f, cfg.fakelag.variance, XOR("%.0f ticks"));
}

void menu::group::rage_desync(std::shared_ptr<layout>& e)
{
	const auto foot_yaw = MAKE("rage.desync.foot_yaw", combo_box, cfg.antiaim.foot_yaw);
	foot_yaw->add({ MAKE("rage.desync.foot_yaw.none", selectable, XOR("none")),
				   MAKE("rage.desync.foot_yaw.static", selectable, XOR("static")),
				   MAKE("rage.desync.foot_yaw.opposite", selectable, XOR("opposite")),
				   MAKE("rage.desync.foot_yaw.jitter", selectable, XOR("jitter")),
				   MAKE("rage.desync.foot_yaw.freestanding", selectable, XOR("freestanding")) });

	ADD_INLINE("foot yaw", foot_yaw,
		MAKE("rage.desync.foot_yaw_flip", checkbox, cfg.antiaim.foot_yaw_flip)->make_tooltip(XOR("flip")));
	ADD_LINE("rage.desync.foot_yaw", MAKE("foot_yaw_label", label, XOR(" ")),
		MAKE("rage.desync.foot_yaw_amount", slider<float>, 0.f, 180.f, cfg.antiaim.foot_yaw_amount, XOR("%.0f°")));
	ADD("desync amount", "rage.desync.limit", slider<float>, 0.f, 60.f, cfg.antiaim.desync_limit, XOR("%.0f°"));

	const auto desync_triggers = MAKE("rage.desync.triggers", combo_box, cfg.antiaim.desync_triggers);
	desync_triggers->add(
		{ MAKE("rage.desync.triggers.jump_impulse", selectable, XOR("on jump impulse")),
		 MAKE("rage.desync.triggers.avoid_extrapolation", selectable, XOR("avoid extrapolation")),
		 MAKE("rage.desync.triggers.avoid_lagcompensation", selectable, XOR("avoid lag compensation")) });
	desync_triggers->allow_multiple = true;
	ADD_INLINE("desync triggers", desync_triggers);

	const auto body_lean = MAKE("rage.desync.body_lean", combo_box, cfg.antiaim.body_lean);
	body_lean->add({ MAKE("rage.desync.body_lean.none", selectable, XOR("none")),
					MAKE("rage.desync.body_lean.static", selectable, XOR("static")),
					MAKE("rage.desync.body_lean.jitter", selectable, XOR("jitter")),
					MAKE("rage.desync.body_lean.freestanding", selectable, XOR("freestanding")) });
	ADD_INLINE("body lean", body_lean,
		MAKE("rage.desync.body_lean_flip", checkbox, cfg.antiaim.body_lean_flip)->make_tooltip(XOR("flip")));
	ADD("body lean amount", "rage.desync.body_lean_amount", slider<float>, 0.f, 100.f, cfg.antiaim.body_lean_amount,
		XOR("%.0f%%"));
	ADD("ensure body lean", "rage.desync.ensure_body_lean", checkbox, cfg.antiaim.ensure_body_lean);
}

std::shared_ptr<layout> menu::group::rage_weapon(const std::string& group, int num, const vec2& size, bool vis)
{
	auto group_container = MAKE_RUNTIME(XOR("rage.weapon_tab.") + group, layout, vec2{}, size, s_justify);
	group_container->make_not_clip();
	group_container->is_visible = vis;
	group_container->add(make_groupbox(
		XOR("rage.weapon.") + group, XOR("weapon (") + group + XOR(")"), size - vec2{0.f, 4.f},
		[num, group](std::shared_ptr<layout>& e)
		{
			const auto group_n = XOR("rage.weapon.") + group + XOR(".");

			const auto auto_scope =
				MAKE_RUNTIME(group_n + XOR("auto_scope"), checkbox, cfg.rage.weapon_cfgs[num].auto_scope);
			ADD_INLINE("auto scope", auto_scope);

			const auto secure_point =
				MAKE_RUNTIME(group_n + XOR("secure_point"), combo_box, cfg.rage.weapon_cfgs[num].secure_point);
			secure_point->add({
				MAKE_RUNTIME(group_n + XOR("secure_point.disabled"), selectable, XOR("disabled"),
							 cfg_t::secure_point_disabled),
				MAKE_RUNTIME(group_n + XOR("secure_point.default"), selectable, XOR("default"),
							 cfg_t::secure_point_default),
				MAKE_RUNTIME(group_n + XOR("secure_point.prefer"), selectable, XOR("prefer"),
							 cfg_t::secure_point_prefer),
				MAKE_RUNTIME(group_n + XOR("secure_point.force"), selectable, XOR("force"), cfg_t::secure_point_force),
				});

			const auto secure_point_enable =
				MAKE_RUNTIME(group_n + XOR("secure_fast_fire"), checkbox, cfg.rage.weapon_cfgs[num].secure_fast_fire);
			secure_point_enable->make_tooltip(XOR("secure fast fire"));
			secure_point_enable->hotkey_type = hkt_none;

			ADD_INLINE("secure point", secure_point, secure_point_enable);

			const auto delay_shot =
				MAKE_RUNTIME(group_n + XOR("delay_shot"), checkbox, cfg.rage.weapon_cfgs[num].delay_shot);
			delay_shot->make_tooltip(XOR("delays shot while player is not in danger"));
			delay_shot->hotkey_type = hkt_none;
			ADD_INLINE("delay shot", delay_shot);

			const auto dynamic =
				MAKE_RUNTIME(group_n + XOR("dynamic"), checkbox, cfg.rage.weapon_cfgs[num].dynamic_min_dmg);
			dynamic->make_tooltip(
				XOR("dynamic mode. attempts to deal as much damage as possible and lowers to set value over time"));
			dynamic->hotkey_type = hkt_none;

			ADD_RUNTIME("hit chance", group_n + XOR("hit_chance"), slider<float>, 0.f, 100.f,
				cfg.rage.weapon_cfgs[num].hitchance_value, XOR("%.0f%%"));
			ADD_INLINE("minimal damage",
				MAKE_RUNTIME(group_n + XOR("minimal_damage"), slider<float>, 0.f, 100.f,
					cfg.rage.weapon_cfgs[num].min_dmg, XOR("%.0fhp")),
				dynamic);

			ADD_INLINE("damage override",
				MAKE_RUNTIME(group_n + XOR("minimal_damage_override"), slider<float>, 0.f, 100.f,
					cfg.rage.weapon_cfgs[num].min_dmg_override, XOR("%.0fhp")));

			const auto hitboxes = MAKE_RUNTIME(group_n + XOR("hitboxes"), combo_box, cfg.rage.weapon_cfgs[num].hitbox);
			hitboxes->allow_multiple = true;
			hitboxes->add({
				MAKE_RUNTIME(group_n + XOR("hitboxes.head"), selectable, XOR("head")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.arms"), selectable, XOR("arms")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.upper_body"), selectable, XOR("upper body")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.lower_body"), selectable, XOR("lower body")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.legs"), selectable, XOR("legs")),
				});

			ADD_INLINE("hitboxes", hitboxes);
			const auto avoid_hitboxes =
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes"), combo_box, cfg.rage.weapon_cfgs[num].avoid_hitbox);
			avoid_hitboxes->allow_multiple = true;
			avoid_hitboxes->add({
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.head"), selectable, XOR("head")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.arms"), selectable, XOR("arms")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.upper_body"), selectable, XOR("upper body")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.lower_body"), selectable, XOR("lower body")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.legs"), selectable, XOR("legs")),
				});

			ADD_INLINE("avoid insecure hitboxes", avoid_hitboxes);

			const auto lethal = MAKE_RUNTIME(group_n + XOR("lethal"), combo_box, cfg.rage.weapon_cfgs[num].lethal);
			lethal->allow_multiple = true;
			lethal->add({ MAKE_RUNTIME(group_n + XOR("lethal.secure_points"), selectable, XOR("force secure points")),
						 MAKE_RUNTIME(group_n + XOR("lethal.body_aim"), selectable, XOR("force body aim")),
						 MAKE_RUNTIME(group_n + XOR("lethal.limbs"), selectable, XOR("avoid limbs")) });

			ADD_INLINE("when lethal", lethal);

			ADD_RUNTIME("multipoint", group_n + XOR("multipoint"), slider<float>, 0.f, 100.f,
				cfg.rage.weapon_cfgs[num].multipoint, XOR("%.0f%%"));

			if (num == grp_general)
				return;

			cfg.rage.weapon_cfgs[num].on_anything_changed = [group](bool status)
				{
					const auto box_deco = ctx->find<gui::group>(hash(XOR("rage.weapon.") + group));
					if (!box_deco)
						return;

					if (!status)
						box_deco->warning = XOR("not configured. general config applies!");
					else
						box_deco->warning.clear();
				};
		}));

	return group_container;
}
