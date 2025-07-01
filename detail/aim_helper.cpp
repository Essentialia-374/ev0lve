
#include <base/debug_overlay.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/dispatch_queue.h>
#include <detail/shot_tracker.h>
#include <hacks/antiaim.h>
#include <hacks/rage.h>
#include <hacks/tickbase.h>
#include <sdk/client_entity_list.h>
#include <sdk/cs_player.h>
#include <sdk/cs_player_resource.h>
#include <sdk/debug_overlay.h>
#include <sdk/surface_props.h>
#include <sdk/weapon.h>
#include <sdk/weapon_system.h>
#include <unordered_set>
#include <util/poly.h>

using namespace sdk;
using namespace hacks;

namespace detail
{
namespace aim_helper
{
static std::vector<seed> precomputed_seeds = {};

int32_t hitbox_to_hitgroup(cs_player_t::hitbox box)
{
	switch (box)
	{
	case cs_player_t::hitbox::head:
	case cs_player_t::hitbox::neck:
		return hitgroup_head;
	case cs_player_t::hitbox::upper_chest:
	case cs_player_t::hitbox::chest:
	case cs_player_t::hitbox::thorax:
		return hitgroup_chest;
	case cs_player_t::hitbox::body:
	case cs_player_t::hitbox::pelvis:
		return hitgroup_stomach;
	case cs_player_t::hitbox::left_forearm:
	case cs_player_t::hitbox::left_upper_arm:
	case cs_player_t::hitbox::left_hand:
		return hitgroup_leftarm;
	case cs_player_t::hitbox::right_forearm:
	case cs_player_t::hitbox::right_upper_arm:
	case cs_player_t::hitbox::right_hand:
		return hitgroup_rightarm;
	case cs_player_t::hitbox::left_calf:
	case cs_player_t::hitbox::left_foot:
	case cs_player_t::hitbox::left_thigh:
		return hitgroup_leftleg;
	case cs_player_t::hitbox::right_calf:
	case cs_player_t::hitbox::right_foot:
	case cs_player_t::hitbox::right_thigh:
		return hitgroup_rightleg;
	default:
		break;
	}

	return hitgroup_generic;
}

bool is_limbs_hitbox(cs_player_t::hitbox box) { return box >= cs_player_t::hitbox::right_thigh; }

bool is_body_hitbox(cs_player_t::hitbox box)
{
	return box == cs_player_t::hitbox::body || box == cs_player_t::hitbox::pelvis;
}

bool is_multipoint_hitbox(cs_player_t::hitbox box)
{
	if (!is_aim_hitbox_rage(box))
		return false;

	return box != cs_player_t::hitbox::left_thigh && box != cs_player_t::hitbox::right_thigh &&
		   box != cs_player_t::hitbox::left_foot && box != cs_player_t::hitbox::right_foot &&
		   box != cs_player_t::hitbox::body;
}

bool is_aim_hitbox_legit(cs_player_t::hitbox box)
{
	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.legit.hack.hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((hitgroup == hitgroup_leftarm || hitgroup == hitgroup_rightarm) &&
		cfg.legit.hack.hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && cfg.legit.hack.hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.legit.hack.hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return ((hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) &&
			cfg.legit.hack.hitbox.get().test(cfg_t::aim_legs));
}

bool is_aim_hitbox_rage(cs_player_t::hitbox box)
{
	if (GET_CONVAR_INT("mp_damage_headshot_only"))
		return box == cs_player_t::hitbox::head && cfg.rage.hack.hitbox.get().test(cfg_t::aim_head);

	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.rage.hack.hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((box == cs_player_t::hitbox::left_upper_arm || box == cs_player_t::hitbox::right_upper_arm) &&
		cfg.rage.hack.hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && box != cs_player_t::hitbox::chest && box != cs_player_t::hitbox::thorax &&
		cfg.rage.hack.hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.rage.hack.hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return (hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) && box != cs_player_t::hitbox::left_thigh &&
		   box != cs_player_t::hitbox::right_thigh && cfg.rage.hack.hitbox.get().test(cfg_t::aim_legs);
}

bool should_avoid_hitbox(cs_player_t::hitbox box)
{
	if (GET_CONVAR_INT("mp_damage_headshot_only"))
		return false;

	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((box == cs_player_t::hitbox::left_upper_arm || box == cs_player_t::hitbox::right_upper_arm) &&
		cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && box != cs_player_t::hitbox::chest &&
		cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return (hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) && box != cs_player_t::hitbox::left_thigh &&
		   box != cs_player_t::hitbox::right_thigh && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_legs);
}

bool is_shooting(sdk::user_cmd *cmd)
{
	const auto weapon_handle = game->local_player->get_active_weapon();

	if (!weapon_handle)
		return false;

	const auto wpn =
		reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(weapon_handle));

	if (!wpn)
		return false;

	const auto is_zeus = wpn->get_item_definition_index() == item_definition_index::weapon_taser;
	const auto is_secondary = (!is_zeus && wpn->get_weapon_type() == weapontype_knife) ||
							  wpn->get_item_definition_index() == item_definition_index::weapon_shield;

	auto shooting = false;

	if (wpn->is_grenade())
	{
		const auto &prev = pred.info[(cmd->command_number - 1) % input_max];
		shooting = prev.sequence == cmd->command_number - 1 && prev.throw_time > 0.f && wpn->get_throw_time() == 0.f;
	}
	else if (is_secondary)
		shooting = (pred.had_attack || pred.had_secondary_attack) && pred.can_shoot;
	else
		shooting = pred.had_attack && pred.can_shoot;

	return shooting;
}

float get_hitchance(cs_player_t *const player)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto hc = cfg.rage.hack.hitchance_value.get() * .01f;

	if (wpn)
	{
		if (wpn->get_item_definition_index() == item_definition_index::weapon_taser)
			return game->local_player->is_on_ground() ? hc : hc / 2.f;
		if (wpn->get_item_definition_index() == item_definition_index::weapon_ssg08 &&
			!game->local_player->is_on_ground() && wpn->get_inaccuracy() < scout_air_accuracy)
			return 0.f;
	}

	if (hc < .01f)
		return 0.f;

	const auto &entry = GET_PLAYER_ENTRY(player);
	return std::clamp(hc + min(entry.spread_miss, 3) / float(wpn->get_cs_weapon_data()->maxclip1), 0.f, 1.f);
}

int32_t get_adjusted_health(cs_player_t *const player)
{
	return max(player->get_player_health() - shot_track.calculate_health_correction(player), 1);
}

float get_mindmg(cs_player_t *const player, const std::optional<cs_player_t::hitbox> hitbox, const bool approach)
{
	const auto &entry = GET_PLAYER_ENTRY(player);
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto info = wpn->get_cs_weapon_data();
	const auto target_min = (cfg.rage.damage_override.get() ?
		cfg.rage.hack.min_dmg_override.get() :
		cfg.rage.hack.min_dmg.get()) / static_cast<float>(info->ibullets);

	if (!wpn || wpn->get_item_definition_index() == item_definition_index::weapon_taser ||
		!cfg.rage.hack.dynamic_min_dmg.get())
		return target_min;

	if (wpn->get_item_definition_index() == item_definition_index::weapon_shield)
		return 1.f;

	const auto hitgroup = hitbox_to_hitgroup(hitbox.value_or(cs_player_t::hitbox::pelvis));
	const auto wpn_dmg =
		max(trace.scale_damage(player, static_cast<float>(info->idamage), info->flarmorratio, hitgroup) - 1.f, 0);

	if (target_min > wpn_dmg)
		return wpn_dmg;

	if (!approach)
		return target_min;

	const auto target_time = std::clamp(entry.aimbot.target_time * 2.f, 0.f, .25f);
	return wpn_dmg - 4.f * target_time * (wpn_dmg - target_min);
}

std::vector<cs_player_t *> get_closest_targets()
{
	constexpr auto close_fov = 20.f;

	auto current_fov = FLT_MAX;
	std::vector<cs_player_t *> targets;

	if (game->local_player && game->local_player->is_alive())
		game->client_entity_list->for_each_player(
			[&targets, &current_fov](cs_player_t *const player)
			{
				if (!player->is_valid(true) || !player->is_enemy() || game->local_player == player)
					return;

				if (player->is_dormant() &&
					GET_PLAYER_ENTRY(player).visuals.last_update <
						TIME_TO_TICKS(game->engine_client->get_last_timestamp()) - TIME_TO_TICKS(5.f))
					return;

				const auto fov = get_fov_simple(game->engine_client->get_view_angles(),
												game->local_player->get_eye_position(), player->get_eye_position());

				if (current_fov > fov || fov < close_fov)
				{
					if (current_fov >= close_fov && fov < close_fov || fov >= close_fov)
						targets.clear();
					targets.push_back(player);
					current_fov = fov;
				}
			});

	return targets;
}

bool in_range(vec3 start, vec3 end)
{
	constexpr auto ray_extension = 12.f;

	ray r{};
	trace_no_player_filter filter{};

	const auto length = (end - start).length();
	const auto direction = (end - start).normalize();

	auto distance = 0.f, thickness = 0.f;
	auto penetrations = 5;

	while (distance <= length)
	{
		if (penetrations < 0)
			return false;

		distance += ray_extension;
		r.init(start + direction * distance, end);
		game_trace tr{};
		game->engine_trace->trace_ray(r, mask_shot_hull, &filter, &tr);

		if (tr.fraction == 1.f)
			return true;

		penetrations--;
		thickness += ray_extension;
		distance += (end - r.start).length() * tr.fraction + ray_extension;

		while (distance <= length)
		{
			auto check = start + direction * distance;
			if (!!(game->engine_trace->get_point_contents_world_only(check, mask_shot_hull) & mask_shot_hull))
				thickness += ray_extension;
			else
			{
				thickness = 0.f;
				break;
			}

			if (thickness >= 90.f)
				return false;

			distance += ray_extension;
		}
	}

	return true;
}

std::optional<sdk::vec3> get_hitbox_position(cs_player_t *const player, const cs_player_t::hitbox id,
											 const mat3x4 *const bones)
{
	const auto studio_model = player->get_studio_hdr()->hdr;

	if (studio_model)
	{
		const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(id), player->get_hitbox_set());

		if (hitbox)
		{
			auto tmp = angle_matrix(hitbox->rotation);
			tmp = concat_transforms(bones[hitbox->bone], tmp);
			vec3 min{}, max{};
			vector_transform(hitbox->bbmin, tmp, min);
			vector_transform(hitbox->bbmax, tmp, max);
			return (min + max) * .5f;
		}
	}

	return std::nullopt;
}

std::vector<rage_target> perform_hitscan(lag_record* record, const custom_tracing::bullet_safety minimal)
{
	std::vector<rage_target> targets;
	if (!record || !record->player || !game->local_player || !game->local_player->is_alive())
		return targets;

	const auto weapon_handle = game->local_player->get_active_weapon();
	if (!weapon_handle)
		return targets;

	auto* const wpn = reinterpret_cast<weapon_t*>(
		game->client_entity_list->get_client_entity_from_handle(weapon_handle));
	if (!wpn || !record->player->get_studio_hdr())
		return targets;

	// Build the matrix array expected by util::player_intersection
	std::array<mat3x4*, 3> mats{
		record->res_mat[record->res_direction],
		record->res_mat[resolver_max],
		record->res_mat[resolver_min]
	};

	for (auto& m : mats)
		if (!m)
			m = record->res_mat[record->res_direction];

	const auto studio_hdr = record->player->get_studio_hdr()->hdr;
	const util::player_intersection pinter(
		studio_hdr, record->player->get_hitbox_set(), mats);

	// Spread‑aware multipoint adjustment radius (defensive maths)
	const sdk::vec3 local_origin = game->local_player->get_origin();
	const float dist = (record->origin - local_origin).length();
	const float spread_rad = wpn->get_spread() + get_lowest_inaccuray();
	const float corner_rad = DEG2RAD(90.0f) - spread_rad;

	float spread_radius = 0.0f;
	if (std::fabs(corner_rad) > 1e-4f)
		spread_radius = (dist / std::sin(corner_rad)) * std::sin(spread_rad);

	std::array<std::deque<rage_target>, static_cast<size_t>(sdk::cs_player_t::hitbox::max)> tmp_buffers;

	std::vector<dispatch_queue::fn> work;
	constexpr std::size_t hitbox_count =
		sizeof(sdk::cs_player_t::hitboxes) / sizeof(sdk::cs_player_t::hitboxes[0]);
	work.reserve(hitbox_count);

	const sdk::vec3 scan_start = game->local_player->get_eye_position();

	for (const auto hb : sdk::cs_player_t::hitboxes)
	{
		if (!is_aim_hitbox_rage(hb))
			continue;

		work.emplace_back(
			[&, hb]												// safe: dispatch.evaluate runs synchronously
			{
				auto& buf = tmp_buffers[static_cast<size_t>(hb)];
				calculate_multipoint(buf, record, hb, pinter,
					nullptr,
					spread_radius,
					minimal);

				for (auto& t : buf)
				{
					t.pen = trace.wall_penetration(
						scan_start, t.position, record,
						t.safety, std::nullopt, nullptr, false, nullptr, t.hitgroup);

					if (t.pen.did_hit)
						t.position = t.pen.end;					// align with real post‑penetration end‑point
				}
			});
	}

	dispatch.evaluate(work);

	size_t total_expected = 0;
	for (const auto& buf : tmp_buffers)
		total_expected += buf.size();
	targets.reserve(total_expected);

	for (auto& buf : tmp_buffers)
		std::move(buf.begin(), buf.end(), std::back_inserter(targets));

	return targets;
}

void detail::aim_helper::calculate_multipoint(std::deque<rage_target>& targets,
	lag_record* record,
	sdk::cs_player_t::hitbox          box,
	const util::player_intersection& pinter,
	sdk::cs_player_t* override_player /* = nullptr */,
	float                             adjustment       /* = 0.f */,
	custom_tracing::bullet_safety     minimal          /* = custom_tracing::bullet_safety::none */)
{
	// defensive checks 
	if (!record || !record->player || !record->player->get_studio_hdr())
		return;

	const auto studio_model = record->player->get_studio_hdr()->hdr;
	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(box),
		record->player->get_hitbox_set());
	if (!hitbox)
		return;

	const auto wpn = reinterpret_cast<weapon_t*>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn)
		return;

	// basic world‑space data
	sdk::vec3 bb_min_ws{}, bb_max_ws{};
	vector_transform(hitbox->bbmin, record->mat[hitbox->bone], bb_min_ws);
	vector_transform(hitbox->bbmax, record->mat[hitbox->bone], bb_max_ws);
	sdk::vec3 center = (bb_min_ws + bb_max_ws) * 0.5f;

	const bool is_fake = record->player->is_fake_player();
	const bool is_knife = wpn->is_knife();

	if (is_fake)
		minimal = custom_tracing::bullet_safety::none;

	// quick damage gate
	if (!override_player && !is_knife)
	{
		const auto  info = wpn->get_cs_weapon_data();
		const float dist_scaled_dmg = info->idamage *
			std::pow(info->flrangemodifier,
				(center - game->local_player->get_eye_position()).length() * 0.002f);

		const float min_dmg_needed =
			min(get_mindmg(record->player, box), get_adjusted_health(record->player));

		if (trace.scale_damage(record->player, dist_scaled_dmg, info->flarmorratio, hitbox->group) + 2.0f
			< min_dmg_needed)
			return;
	}

	// radius scale factor (rs)
	float rs = 0.975f;
	if (!override_player)
	{
		auto& entry = GET_PLAYER_ENTRY(record->player);
		if (entry.spread_miss > 2)  rs *= 1.f - (entry.spread_miss - 2) * 0.20f;
		if (is_limbs_hitbox(box))   rs *= 0.75f;
		if (!game->cl_lagcompensation || !game->cl_predict)
			rs *= 0.80f;
	}
	rs *= 0.5f + 0.5f * cfg.rage.hack.multipoint.get() * 0.01f;
	rs = std::clamp(rs - adjustment * 0.10f, 0.05f, 1.0f);

	// fast path for non‑capsule / knife hitboxes
	if (hitbox->radius < 0.f || is_knife)
	{
		sdk::vec3 point = center;
		if (is_knife)
		{
			const float r = hitbox->radius < 0.f ? 3.f : hitbox->radius;
			sdk::angle ang = calc_angle(game->local_player->get_eye_position(), center);
			sdk::vec3  fwd; angle_vectors(ang, fwd);
			point -= fwd * r * rs;
		}
		else if (minimal != custom_tracing::bullet_safety::none)
			return;

		targets.emplace_back(point, record, false, center, box, hitbox->group, 0.f);
		return;
	}

	// build a local orthonormal basis tied to view direction
	const sdk::vec3 eye = override_player ? override_player->get_eye_position()
		: game->local_player->get_eye_position();

	sdk::angle view_ang = calc_angle(eye, center);
	sdk::vec3  n;  angle_vectors(view_ang, n); // forward

	const sdk::vec3 world_up(0.f, 0.f, 1.f);
	sdk::vec3       u = n.cross(std::fabs(n.z) > 0.99f ? sdk::vec3(0.f, 1.f, 0.f) : world_up); // right

	if (u.length_sqr() < 1e-6f) 
		u = sdk::vec3(1.f, 0.f, 0.f); // fallback
	
	u.normalize();
	const sdk::vec3 v = u.cross(n); // up in hit‑plane

	// thread‑local scratch buffers (mirrors original design)
	static thread_local util::circular_buffer<sdk::vec3> projected_pts[3][resolver_direction_max];
	static thread_local util::circular_buffer<sdk::vec3> mapped_pts[3][resolver_direction_max];

	auto clear_buffers = []()
		{
			for (auto& lvl : projected_pts) for (auto& buf : lvl) buf.clear();
			for (auto& lvl : mapped_pts)    for (auto& buf : lvl) buf.clear();
		};
	clear_buffers();

	// helpers 
	const auto point_to_world =
		[&](const sdk::vec3& p_ws0, const sdk::vec3& local) -> sdk::vec3
		{
			const sdk::vec3 d(1.f, local.x, local.y);   // local = (1, x, y)
			return sdk::vec3(sdk::vec3(p_ws0.x, u.x, v.x).dot(d),
				sdk::vec3(p_ws0.y, u.y, v.y).dot(d),
				sdk::vec3(p_ws0.z, u.z, v.z).dot(d));
		};

	const auto is_hit =
		[&](const sdk::vec3& pos, bool secure) -> bool
		{
			sdk::ray r; r.init(eye, pos);
			return pinter.rank(pinter.trace_hitgroup(r, secure)) == pinter.rank(hitbox->group);
		};

	// generate a ring of points on the capsule caps
	constexpr float mid = 0.5f;
	constexpr float dt = 0.125f;
	constexpr float one_third = mid - dt;
	constexpr float two_third = mid + dt;

	const float rad_in = hitbox->radius * 0.925f;

	auto make_ring =
		[&](util::circular_buffer<sdk::vec3>& ring,
			const sdk::vec3& min_ws,
			const sdk::vec3& max_ws)
		{
			ring.reserve(24);

			const sdk::vec3 right = u * rad_in;
			const sdk::vec3 top = sdk::vec3(0.f, 0.f, 1.f) * rad_in;
			const sdk::vec3 left = n.cross(sdk::vec3(0.f, 0.f, -1.f)) * rad_in;
			const sdk::vec3 bot = top * -1.f;

			const auto push_pair = [&](const sdk::vec3& p)
				{
					*ring.push_front() = min_ws + p;
					*ring.push_front() = max_ws + p;
				};

			push_pair(right); push_pair(top); push_pair(left); push_pair(bot);

			push_pair(interpolate(right, top, one_third).normalize() * rad_in);
			push_pair(interpolate(right, top, two_third).normalize() * rad_in);
			push_pair(interpolate(right, bot, one_third).normalize() * rad_in);
			push_pair(interpolate(right, bot, two_third).normalize() * rad_in);

			push_pair(interpolate(left, top, one_third).normalize() * rad_in);
			push_pair(interpolate(left, top, two_third).normalize() * rad_in);
			push_pair(interpolate(left, bot, one_third).normalize() * rad_in);
			push_pair(interpolate(left, bot, two_third).normalize() * rad_in);

			for (auto& p : ring)           // flatten onto cross‑section plane
				p -= n * (p - center).dot(n);
		};

	// build projected + mapped point clouds for various resolver states
	for (int lvl = 0; lvl < 3; ++lvl)
	{
		const bool use_prev_mat = (lvl == 1);
		const bool use_prev_rec = (lvl == 2);
		if (lvl && override_player)               break;
		if (use_prev_mat && !record->has_previous_matrix) continue;
		if (use_prev_rec && !record->previous)            break;

		for (int dir = 0; dir < resolver_direction_max; ++dir)
		{
			if ((is_fake || override_player) && dir != resolver_networked) continue;

			bool primary =
				(dir == record->res_direction && !use_prev_rec)
				|| (dir == resolver_networked && !use_prev_rec && !use_prev_mat)
				|| dir == resolver_center
				|| dir == resolver_min
				|| dir == resolver_max
				|| dir == resolver_min_min
				|| dir == resolver_max_max
				|| dir == resolver_min_extra
				|| dir == resolver_max_extra;

			if (!primary) continue;

			const mat3x4& bone =
				use_prev_rec ? record->previous->res_mat[dir][hitbox->bone]
				: use_prev_mat ? record->previous_res_mat[dir][hitbox->bone]
				: record->res_mat[dir][hitbox->bone];

			vector_transform(hitbox->bbmin, bone, bb_min_ws);
			vector_transform(hitbox->bbmax, bone, bb_max_ws);

			make_ring(projected_pts[lvl][dir], bb_min_ws, bb_max_ws);

			const sdk::vec3 p0 = projected_pts[0][resolver_networked][0];
			auto& mapped = mapped_pts[lvl][dir];
			mapped.resize(projected_pts[lvl][dir].size());

			for (size_t k = 0; k < projected_pts[lvl][dir].size(); ++k)
			{
				const sdk::vec3 d = projected_pts[lvl][dir][k] - p0;
				mapped[k] = sdk::vec3(d.dot(u), d.dot(v), 0.f);
			}
			if (!override_player)
				util::monotone_chain(mapped);
		}
	}

	// helper to turn intersection polygon into final 3‑D points
	const auto extract_points =
		[&](util::convex_polygon poly,
			custom_tracing::bullet_safety safety) -> bool
		{
			if (poly.size() < 3) return false;

			sdk::vec3 l = poly.front(), r = l, t = l, b = l;
			for (auto& p : poly)
			{
				if (p.x < l.x) l = p;
				if (p.x > r.x) r = p;
				if (p.y > t.y) t = p;
				if (p.y < b.y) b = p;
			}

			const sdk::vec3 p0 = projected_pts[0][resolver_networked][0];
			sdk::vec3 ws_l = point_to_world(p0, l);
			sdk::vec3 ws_r = point_to_world(p0, r);
			sdk::vec3 ws_t = point_to_world(p0, t);
			sdk::vec3 ws_b = point_to_world(p0, b);

			sdk::vec3 new_center = (ws_l + ws_r + ws_t + ws_b) * 0.25f;

			auto shrink = [&](sdk::vec3& p_ws) { p_ws = interpolate(new_center, p_ws, rs); };
			shrink(ws_l); shrink(ws_r); if (box == sdk::cs_player_t::hitbox::head) shrink(ws_t);

			auto push_if_valid =
				[&](const sdk::vec3& pt, float dist_sq)
				{
					if (!is_hit(pt, safety > custom_tracing::bullet_safety::none))
						return;
					targets.emplace_back(pt, record, false, new_center, box,
						hitbox->group, 0.f, dist_sq,
						safety != custom_tracing::bullet_safety::none ? util::area(poly) : 0.f,
						is_fake ? custom_tracing::bullet_safety::full : safety);
				};

			push_if_valid(new_center, 0.f);
			push_if_valid(ws_l, (new_center - ws_l).length_sqr());
			push_if_valid(ws_r, (new_center - ws_r).length_sqr());
			if (box == sdk::cs_player_t::hitbox::head)
				push_if_valid(ws_t, (new_center - ws_t).length_sqr());

			return true;
		};

	// initial (networked) intersection pass 
	util::convex_polygon base = mapped_pts[0][record->res_direction];
	if (record->has_previous_matrix)
		base = util::get_intersection_poly(base, mapped_pts[1][record->res_direction]);

	if (!extract_points(base, custom_tracing::bullet_safety::none))
		return;

	// extra‑safety passes
	if (is_fake || (is_limbs_hitbox(box) &&
		!(record->velocity.length2d() < 1.1f ||
			!(record->flags & sdk::cs_player_t::on_ground))))
		return;

	rs *= 0.80f;

	util::convex_polygon secure =
		util::get_intersection_poly(mapped_pts[0][resolver_min], mapped_pts[0][resolver_max]);
	secure = util::get_intersection_poly(secure, mapped_pts[0][resolver_center]);

	if (record->previous)
	{
		secure = util::get_intersection_poly(secure, mapped_pts[2][resolver_min]);
		secure = util::get_intersection_poly(secure, mapped_pts[2][resolver_max]);
		secure = util::get_intersection_poly(secure, mapped_pts[2][resolver_center]);
	}
	if (record->has_previous_matrix)
	{
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_min]);
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_max]);
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_center]);
	}

	if (minimal != custom_tracing::bullet_safety::full)
		extract_points(secure, custom_tracing::bullet_safety::no_roll);

	// full‑safety (worst‑case) pass
	secure = util::get_intersection_poly(secure, mapped_pts[0][resolver_min_min]);
	secure = util::get_intersection_poly(secure, mapped_pts[0][resolver_max_max]);
	secure = util::get_intersection_poly(secure, mapped_pts[0][resolver_min_extra]);
	secure = util::get_intersection_poly(secure, mapped_pts[0][resolver_max_extra]);

	if (record->previous)
	{
		secure = util::get_intersection_poly(secure, mapped_pts[2][resolver_min_min]);
		secure = util::get_intersection_poly(secure, mapped_pts[2][resolver_max_max]);
	}
	if (record->has_previous_matrix)
	{
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_min_min]);
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_max_max]);
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_min_extra]);
		secure = util::get_intersection_poly(secure, mapped_pts[1][resolver_max_extra]);
	}

	extract_points(secure, custom_tracing::bullet_safety::full);
}

void optimize_cornerpoint(rage_target *target)
{
	if (target->dist < 1.f)
		return;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	const auto info = wpn->get_cs_weapon_data();
	const auto eye = game->local_player->get_eye_position();

	auto new_point = *target;
	const auto shots_to_kill = target->get_shots_to_kill();

	for (auto i = 0; i < 6; i++)
	{
		auto next_point = *target;
		next_point.position = interpolate(target->position, target->center, i / 6.f);
		next_point.pen = trace.wall_penetration(eye, next_point.position, target->record, target->safety, std::nullopt,
												nullptr, false, nullptr, target->hitgroup);
		if (!next_point.pen.did_hit || next_point.pen.damage < min(get_mindmg(target->record->player, target->hitbox),
																   get_adjusted_health(target->record->player)))
			continue;
		if (new_point.get_shots_to_kill() <= shots_to_kill)
			new_point = next_point;
	}

	if (new_point.position == target->position)
		return;

	new_point.position = interpolate(new_point.position, target->position, .5f);
	new_point.pen = trace.wall_penetration(eye, new_point.position, target->record, target->safety, std::nullopt,
										   nullptr, false, nullptr, target->hitgroup);
	if (!new_point.pen.did_hit || new_point.pen.damage < min(get_mindmg(target->record->player, target->hitbox),
															 get_adjusted_health(target->record->player)))
		return;
	if (new_point.get_shots_to_kill() <= shots_to_kill)
		*target = new_point;
}

float get_lowest_inaccuray()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto info = wpn->get_cs_weapon_data();

	if (game->local_player->get_flags() & cs_player_t::ducking)
		return (wpn->is_scoped_weapon() && (cfg.rage.hack.auto_scope.get() || wpn->get_zoom_level() != 0))
				   ? info->flinaccuracycrouchalt
				   : info->flinaccuracycrouch;

	return (wpn->is_scoped_weapon() && (cfg.rage.hack.auto_scope.get() || wpn->get_zoom_level() != 0))
			   ? info->flinaccuracystandalt
			   : info->flinaccuracystand;
}

bool has_full_accuracy()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	return wpn->get_inaccuracy() == 0.f || fabsf(wpn->get_inaccuracy() - get_lowest_inaccuray()) < .001f;
}

float calculate_hitchance(rage_target *target, bool full_accuracy)
{
	if (precomputed_seeds.empty())
		return 0.f;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return 0.f;

	if (!target->record->player->get_studio_hdr())
		return 0.f;

	constexpr auto hg_equal_or_better = [](int32_t hitgroup, int32_t second)
	{
		if (hitgroup != second)
		{
			if (second == hitgroup_gear || second == hitgroup_generic)
				return false;
		}

		switch (hitgroup)
		{
		case hitgroup_head:
			if (second != hitgroup_head)
				return false;
			break;
		case hitgroup_neck:
		case hitgroup_chest:
			if (second > hitgroup_stomach && second != hitgroup_neck)
				return false;
			break;
		case hitgroup_stomach:
			if (second == hitgroup_chest || second > hitgroup_stomach)
				return false;
			break;
		case hitgroup_leftarm:
		case hitgroup_rightarm:
			if (second > hitgroup_rightarm && second != hitgroup_neck)
				return false;
			break;
		default:
			break;
		}

		return true;
	};

	const auto studio_model = target->record->player->get_studio_hdr()->hdr;

	const auto start = game->local_player->get_eye_position();
	const auto info = wpn->get_cs_weapon_data();
	const auto has_best_speed =
		game->local_player->get_velocity().length() <=
		(wpn->get_item_definition_index() == item_definition_index::weapon_revolver ? 180.f : wpn->get_max_speed()) /
				3.f -
			1.f;
	const auto true_accuracy = full_accuracy ? true : has_full_accuracy();
	const auto min_damage = get_mindmg(target->record->player, target->hitbox);
	const auto health = get_adjusted_health(target->record->player);

	// calculate inaccuracy and spread.
	const auto weapon_inaccuracy = full_accuracy ? get_lowest_inaccuray() : wpn->get_inaccuracy();
	const auto weapon_spread = info->flspread;

	// calculate angle.
	const auto aim_angle = calc_angle(start, target->position);
	vec3 forward, right, up;
	angle_vectors(aim_angle, forward, right, up);

	// setup calculation parameters.
	vec3 total_spread, spread_angle, end;
	float inaccuracy, spread, total_x, total_y;
	seed *s;

	// are we playing on a no-spread server?
	const auto is_spread = weapon_inaccuracy > 0.f;
	const auto total = is_spread ? total_seeds : 1;

	// setup all traces.
	std::vector<dispatch_queue::fn> calls;
	calls.reserve(total);
	std::vector<custom_tracing::wall_pen> traces(total);

	std::array<mat3x4 *, 3> mats = {target->record->res_mat[target->record->res_direction], nullptr, nullptr};
	if (target->record->res_direction != resolver_max && target->record->res_direction != resolver_min)
	{
		mats[1] = target->record->res_mat[resolver_max];
		mats[2] = target->record->res_mat[resolver_min];
	}
	else
	{
		if (target->record->res_direction != resolver_max)
			mats[1] = target->record->res_mat[resolver_max];
		else
			mats[1] = target->record->res_mat[resolver_min];
	}
	const auto pinter = util::player_intersection(studio_model, target->record->player->get_hitbox_set(), mats);
	const auto peek_exploit_ended = (tickbase.fast_fire || tickbase.hide_shot) &&
									game->local_player->get_tick_base() >= tickbase.lag.first - 2 &&
									!aa.is_lag_hittable();
	const auto is_zeus = wpn->get_item_definition_index() == item_definition_index::weapon_taser;
	const auto ignore_safety = has_best_speed || true_accuracy || is_zeus || peek_exploit_ended;

	if (is_spread)
		for (auto i = 0; i < total; i++)
		{
			// get seed.
			s = &precomputed_seeds[i];

			// calculate spread.
			inaccuracy = s->inaccuracy;
			spread = s->spread;

			// correct spread value for different weapons.
			if (wpn->get_item_definition_index() == item_definition_index::weapon_negev &&
				wpn->get_recoil_index() < 3.f)
				for (auto j = 3; j > static_cast<int32_t>(wpn->get_recoil_index()); j--)
					inaccuracy *= inaccuracy;

			inaccuracy *= weapon_inaccuracy;
			spread *= weapon_spread;
			total_x = (s->cos_inaccuracy * inaccuracy) + (s->cos_spread * spread);
			total_y = (s->sin_inaccuracy * inaccuracy) + (s->sin_spread * spread);
			total_spread = (forward + right * total_x + up * total_y).normalize();

			// calculate angle with spread applied.
			vector_angles(total_spread, spread_angle);

			// calculate end point of trace.
			angle_vectors(spread_angle, end);
			end = start + end.normalize() * info->range;

			// insert call instruction.
			calls.push_back(
				[&traces, &start, end, &target, i, is_zeus, &pinter, ignore_safety, &hg_equal_or_better]()
				{
					if (is_zeus)
					{
						ray r;
						r.init(start, end);

						if (pinter.trace_hitgroup(r, target->safety > custom_tracing::bullet_safety::none) ==
							hitgroup_gear)
							return;
					}

					if (!ignore_safety)
					{
						ray r;
						r.init(start, end);

						if (!hg_equal_or_better(
								target->hitgroup,
								pinter.trace_hitgroup(r, target->safety > custom_tracing::bullet_safety::none)))
							return;
					}

					traces[i] = trace.wall_penetration(start, end, target->record, custom_tracing::bullet_safety::none,
													   std::nullopt, nullptr, false, nullptr);
				});
		}
	else
		traces[0] = trace.wall_penetration(start, target->position, target->record, custom_tracing::bullet_safety::none,
										   std::nullopt, nullptr, false, nullptr);

	// dispatch traces to tracing pool.
	dispatch.evaluate(calls);

	// calculate statistics.
	auto total_hits = 0;

	for (const auto &tr : traces)
	{
		if (!tr.did_hit)
			continue;

		if (is_zeus)
		{
			if (tr.damage >= 100.f)
				total_hits++;

			continue;
		}

		if (!true_accuracy && !hg_equal_or_better(target->hitgroup, tr.hitgroup))
			continue;

		if (true_accuracy && tr.impact_count == 1 || tr.damage >= min_damage || tr.damage >= health)
			total_hits++;
	}

	// return result.
	return min(static_cast<float>(total_hits * info->ibullets) / static_cast<float>(total), 1.f);
}

bool should_prefer_record(std::optional<rage_target>& target, std::optional<rage_target>& alternative, bool compare_hc /* = false */)
{
	constexpr float kHealthBuffer = health_buffer;  // 6 HP guard band
	constexpr float kSafetySizeMargin = 5.0f;           
	constexpr float kHitDistTolerance = 0.30f;          
	constexpr int   kShotTolerance = 1;              

	if (!alternative || !alternative->pen.did_hit)               
		return false;

	if (!target || !target->pen.did_hit)                        
	{
		alternative->best_to_kill = alternative->get_shots_to_kill();
		return true;
	}

	const int alt_shots = alternative->get_shots_to_kill();
	const int cur_shots = target->get_shots_to_kill();

	if (alt_shots + kShotTolerance < cur_shots)                  
	{
		alternative->best_to_kill = min(alt_shots, target->best_to_kill);
		return true;
	}
	if (cur_shots + kShotTolerance < alt_shots)                 
		return false;

	// Bullet‑safety level 
	if (alternative->pen.safety != target->pen.safety)
	{
		if (static_cast<int>(alternative->pen.safety) >
			static_cast<int>(target->pen.safety))
		{
			alternative->best_to_kill = min(alt_shots, target->best_to_kill);
			return true;
		}
		return false;
	}

	// Safety surface area
	if (std::fabs(alternative->safety_size - target->safety_size) > kSafetySizeMargin)
	{
		if (alternative->safety_size > target->safety_size)
		{
			alternative->best_to_kill = min(alt_shots, target->best_to_kill);
			return true;
		}
		return false;
	}

	// Raw damage (buffered) 
	if (alternative->pen.damage > target->pen.damage + kHealthBuffer)
	{
		alternative->best_to_kill = min(alt_shots, target->best_to_kill);
		return true;
	}
	if (alternative->pen.damage + kHealthBuffer < target->pen.damage)
		return false;

	//  Hitchance (optional) 
	if (compare_hc && std::fabs(alternative->hc - target->hc) > 0.015f)
	{
		if (alternative->hc > target->hc)
		{
			alternative->best_to_kill = min(alt_shots, target->best_to_kill);
			return true;
		}
		return false;
	}

	// Hit‑area class (capsule vs. bbox)
	bool alt_capsule = false, cur_capsule = false;
	{
		const auto alt_hdr = alternative->record->player->get_studio_hdr();
		const auto cur_hdr = target->record->player->get_studio_hdr();
		if (alt_hdr && cur_hdr)
		{
			const auto alt_hb = alt_hdr->hdr->get_hitbox(
				static_cast<uint32_t>(alternative->pen.hitbox), 0);
			const auto cur_hb = cur_hdr->hdr->get_hitbox(
				static_cast<uint32_t>(target->pen.hitbox), 0);
			if (alt_hb && cur_hb)
			{
				alt_capsule = alt_hb->radius != -1.0f;
				cur_capsule = cur_hb->radius != -1.0f;
			}
		}
	}
	if (alt_capsule != cur_capsule) // prefer capsules / spheres
	{
		if (alt_capsule)
		{
			alternative->best_to_kill = min(alt_shots, target->best_to_kill);
			return true;
		}
		return false;
	}

	// Distance to hit‑box centre
	if (alternative->dist + kHitDistTolerance < target->dist)
	{
		alternative->best_to_kill = min(alt_shots, target->best_to_kill);
		return true;
	}
	if (target->dist + kHitDistTolerance < alternative->dist)
		return false;

	// Record recency
	if (alternative->record->recv_time > target->record->recv_time)
	{
		alternative->best_to_kill = min(alt_shots, target->best_to_kill);
		return true;
	}

	// No decisive advantage, keep current
	return false;
}

bool is_attackable()
{
	if (rag.has_priority_targets())
		return true;

	auto found = false;

	game->client_entity_list->for_each_player(
		[&](cs_player_t *const player)
		{
			if (!player->is_valid() || !player->is_alive() || !player->is_enemy())
				return;

			const auto &entry = GET_PLAYER_ENTRY(player);

			if (entry.hittable || entry.danger)
				found = true;
		});

	return found;
}

bool holds_tick_base_weapon()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return false;

	return wpn->get_item_definition_index() != item_definition_index::weapon_taser &&
		   wpn->get_item_definition_index() != item_definition_index::weapon_fists &&
		   wpn->get_item_definition_index() != item_definition_index::weapon_c4 && !wpn->is_grenade() &&
		   wpn->get_class_id() != class_id::snowball && wpn->get_weapon_type() != weapontype_unknown;
}

void fix_movement(user_cmd *const cmd, angle &wishangle)
{
	vec3 view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	angle_vectors(wishangle, view_fwd, view_right, view_up);
	angle_vectors(cmd->viewangles, cmd_fwd, cmd_right, cmd_up);

	view_fwd.z = view_right.z = cmd_fwd.z = cmd_right.z = 0.f;
	view_fwd.normalize();
	view_right.normalize();
	cmd_fwd.normalize();
	cmd_right.normalize();

	const auto dir = view_fwd * cmd->forwardmove + view_right * cmd->sidemove;
	const auto denom = cmd_right.y * cmd_fwd.x - cmd_right.x * cmd_fwd.y;

	cmd->sidemove = (cmd_fwd.x * dir.y - cmd_fwd.y * dir.x) / denom;
	cmd->forwardmove = (cmd_right.y * dir.x - cmd_right.x * dir.y) / denom;

	normalize_move(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	wishangle = cmd->viewangles;
}

float normalize_float(float angle)
{
	auto revolutions = angle / 360.f;
	if (angle > 180.f || angle < -180.f)
	{
		revolutions = round(abs(revolutions));
		if (angle < 0.f)
			angle = (angle + 360.f * revolutions);
		else
			angle = (angle - 360.f * revolutions);
		return angle;
	}
	return angle;
}
void sin_cos(float radians, float* sine, float* cosine)
{
	*sine = sin(radians);
	*cosine = cos(radians);
}
void angle_vectors(const angle& angles, vec3* forward = nullptr, vec3* right = nullptr, vec3* up = nullptr)
{
	float sr, sp, sy, cr, cp, cy;

	sin_cos(DEG2RAD(angles.x), &sp, &cp);
	sin_cos(DEG2RAD(angles.y), &sy, &cy);
	sin_cos(DEG2RAD(angles.z), &sr, &cr);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;
	}

	if (up)
	{
		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
}

float VectorNormalize(vec3& vec)
{
	const auto sqrlen = vec.length_sqr() + 1.0e-10f;
	float invlen;
	const auto xx = _mm_load_ss(&sqrlen);
	auto xr = _mm_rsqrt_ss(xx);
	auto xt = _mm_mul_ss(xr, xr);
	xt = _mm_mul_ss(xt, xx);
	xt = _mm_sub_ss(_mm_set_ss(3.f), xt);
	xt = _mm_mul_ss(xt, _mm_set_ss(0.5f));
	xr = _mm_mul_ss(xr, xt);
	_mm_store_ss(&invlen, xr);
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
	return sqrlen * invlen;
}

bool is_moving(user_cmd* cmd)
{
	return (cmd->buttons & (user_cmd::forward | user_cmd::back | user_cmd::move_left | user_cmd::move_right)) != 0;
}

void stop_to_speed(float speed, move_data* mv, cs_player_t* player)
{
	/*game->cs_game_movement->friction(mv, player);
	game->cs_game_movement->check_parameters(mv, player);*/

	auto mv_cp = *mv;
	//game->cs_game_movement->walk_move(&mv_cp, player);
	if (mv_cp.velocity.length2d() <= speed)
		return;

	if (mv->velocity.length2d() > speed)
	{
		angle ang{};
		vector_angles(mv->velocity * -1.f, ang);
		ang.y = normalize_float(mv->view_angles.y - ang.y);

		vec3 forward{};
		angle_vectors(ang, &forward);
		forward = forward.to_2d().normalize();

		const auto target_move_len = min(mv->velocity.length2d() - speed, mv->max_speed);
		mv->forward_move = forward.x * target_move_len;
		mv->side_move = forward.y * target_move_len;
		mv->up_move = 0.f;
		return;
	}

	vec3 forward, right;
	angle_vectors(mv->view_angles, &forward, &right);
	forward.z = right.z = 0.f;

	VectorNormalize(forward);
	VectorNormalize(right);

	vec3 wishdir(forward.x * mv->forward_move + right.x * mv->side_move,
		forward.y * mv->forward_move + right.y * mv->side_move, 0.f);
	VectorNormalize(wishdir);

	const auto currentspeed = mv->velocity.dot(wishdir);
	const auto target_accelspeed = speed - mv->velocity.length2d();
	const auto target_move_len = min(currentspeed + target_accelspeed, mv->max_speed);

	const auto speed_squared =
		(mv->forward_move * mv->forward_move) + (mv->side_move * mv->side_move) + (mv->up_move * mv->up_move);
	const auto ratio = target_move_len / sqrt(speed_squared);
	if (ratio < 1.f)
	{
		mv->forward_move *= ratio;
		mv->side_move *= ratio;
		mv->up_move *= ratio;
	}
}

void stop_to_speed(float speed, user_cmd* cmd)
{
	auto local_player = game->local_player;
	if ((speed == 0.f || (pred.original_cmd.forwardmove == 0.f && pred.original_cmd.sidemove == 0.f)) &&
		pred.info[(cmd->command_number - 1) % input_max].velocity.length() < 1.1f)
	{
		if (!is_moving)
		{
			// micromove
			if (local_player->get_flags() & sdk::cs_player_t::flags::on_ground &&
				!(cmd->buttons & user_cmd::jump) && !cfg.antiaim.foot_yaw.get().test(cfg_t::foot_yaw_none) &&
				cfg.antiaim.enable.get() && cmd->command_number > 1 &&
				pred.info[(cmd->command_number - 1) % input_max].flags & sdk::cs_player_t::flags::on_ground)
			{
				auto forwardmove = random_float(1.01f, -1.01f) /* % 2 ? 1.01f : -1.01f*/;
				forwardmove /= local_player->get_duck_amount() * .34f + 1.f - local_player->get_duck_amount();

				cmd->forwardmove = forwardmove;
				cmd->sidemove = 0.f;
			}
		}

		return;
	}

	pred.repredict(&game->input->commands[(cmd->command_number - 1) % input_max]);
	move_data data = game->cs_game_movement->setup_move(local_player, cmd);
	stop_to_speed(speed, &data, local_player);

	cmd->forwardmove = data.forward_move;
	cmd->sidemove = data.side_move;

	const auto forwardmove = (game->client_state->choked_commands % 2 ? -1.01f : 1.01f);

	//cs_game_movement_t::walk_move(&data, local_player);
	if (!cfg.antiaim.foot_yaw.get().test(cfg_t::foot_yaw_none) && local_player->get_velocity().length() <= 1.10f &&
		data.velocity.length() <= 1.10f && local_player->get_flags() & sdk::cs_player_t::flags::on_ground &&
		cfg.antiaim.enable.get())
	{
		if (!is_moving)
		{
			cmd->forwardmove = forwardmove;
			cmd->sidemove = 0.f;
		}
	}

	const auto factor = local_player->get_duck_amount() * .34f + 1.f - local_player->get_duck_amount();
	cmd->forwardmove /= factor;
	cmd->sidemove /= factor;

	pred.repredict(cmd);
}
void slow_movement(user_cmd* const cmd, float target_speed)
{
	stop_to_speed(target_speed, cmd);
	/*pred.repredict(&game->input->commands[(cmd->command_number - 1) % input_max]);
	auto data = game->cs_game_movement->setup_move(game->local_player, cmd);
	auto changed_movement = slow_movement(&data, target_speed);
	std::tie(cmd->forwardmove, cmd->sidemove) = std::tie(data.forward_move, data.side_move);
	normalize_move(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	pred.repredict(cmd);

	if (aa.is_active() && !cfg.antiaim.foot_yaw.get().test(cfg_t::foot_yaw_none) && game->local_player->is_on_ground()
		&& data.velocity.length() <= 1.1f && game->local_player->get_velocity().length() <= 1.f && !changed_movement)
	{
		cmd->forwardmove = (game->client_state->choked_commands % 2 ? -1.01f : 1.01f);
		changed_movement = true;
	}

	if (changed_movement)
	{
		const auto factor = game->local_player->get_duck_amount() * .34f + 1.f - game->local_player->get_duck_amount();
		cmd->forwardmove /= factor;
		cmd->sidemove /= factor;
		normalize_move(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	}*/
}

bool slow_movement(sdk::move_data* const data, float target_speed)
{
	const auto player = reinterpret_cast<cs_player_t*>(game->client_entity_list->get_client_entity_from_handle(data->player_handle));
	const auto velocity = data->velocity;
	const auto current_speed = velocity.length2d();
	const auto stop_speed = max(GET_CONVAR_FLOAT("sv_stopspeed"), current_speed);
	const auto friction = GET_CONVAR_FLOAT("sv_friction") * player->get_surface_friction();
	const auto applied_stop_speed = stop_speed * friction * game->globals->interval_per_tick;
	const auto friction_speed = max(current_speed - applied_stop_speed, 0.f);
	const auto friction_velocity = friction_speed == current_speed ? velocity :
		(vec3(velocity.x, velocity.y, 0.f) * (friction_speed / current_speed));
	auto move_direction = vec3(data->forward_move, data->side_move, 0.f);
	auto next = *data;

	if (friction_velocity.length2d() > target_speed && friction_speed > friction)
	{
		angle ang;
		vector_angles(friction_velocity * -1.f, ang);
		ang.y = data->view_angles.y - ang.y;

		vec3 forward;
		angle_vectors(ang, forward);
		auto max_speed_x = current_speed - target_speed - forward_bounds * friction * game->globals->interval_per_tick;
		auto max_speed_y = current_speed - target_speed - side_bounds * friction * game->globals->interval_per_tick;
		max_speed_x = std::clamp(max_speed_x < 0.f ? friction_speed : forward_bounds, 0.f, forward_bounds);
		max_speed_y = std::clamp(max_speed_y < 0.f ? friction_speed : side_bounds, 0.f, side_bounds);
		std::tie(data->forward_move, data->side_move) = std::make_pair(forward.x * max_speed_x, forward.y * max_speed_y);
		return true;
	}
	else if (game->cs_game_movement->process_movement(player, &next); move_direction.length2d() > 0.f && friction_speed > friction && next.velocity.length() > target_speed)
	{
		vec3 forward, right, up;
		angle_vectors(data->view_angles, forward, right, up);
		forward.z = right.z = 0.f;

		const auto wish = vec3(forward.x * move_direction.x + right.x * move_direction.y,
			forward.y * move_direction.x + right.y * move_direction.y, 0.f);

		const auto move = move_direction;
		move_direction.x = 0.f;
		move_direction.y = 0.f;

		if (wish.length2d() > 0.f)
		{
			const auto normalized_wish = vec3(wish).normalize();
			const auto magnitude = friction_velocity.dot(normalized_wish) + (target_speed - friction_velocity.length2d());

			if (magnitude > 0.f)
			{
				move_direction = move;
				move_direction.normalize();
				move_direction *= magnitude;
			}
		}

		std::tie(data->forward_move, data->side_move) = std::make_pair(move_direction.x, move_direction.y);
		return true;
	}
	else if (friction_speed < applied_stop_speed && target_speed < applied_stop_speed)
		std::tie(data->forward_move, data->side_move) = std::make_pair(0.f, 0.f);

	return false;
}

void build_seed_table()
{
	if (precomputed_seeds.empty())
		for (auto i = 0; i < total_seeds; i++)
		{
			random_seed(i + 1);

			seed a;
			a.inaccuracy = random_float(0.f, 1.f);
			auto p = random_float(0.f, 2.f * sdk::pi);
			a.sin_inaccuracy = sin(p);
			a.cos_inaccuracy = cos(p);
			a.spread = random_float(0.f, 1.f);
			p = random_float(0.f, 2.f * sdk::pi);
			a.sin_spread = sin(p);
			a.cos_spread = cos(p);

			precomputed_seeds.push_back(a);
		}
}

inline int32_t rage_target::get_shots_to_kill()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto health = get_adjusted_health(record->player);
	auto shots = static_cast<int32_t>(ceilf(health / fminf(pen.damage, health)));

	if (tickbase.fast_fire && holds_tick_base_weapon() && cfg.rage.hack.secure_fast_fire.get() &&
		tickbase.compute_current_limit() > 0)
		shots -= static_cast<int32_t>(
			floorf(TICKS_TO_TIME(tickbase.compute_current_limit()) / (.75f * wpn->get_cs_weapon_data()->cycle_time)));

	return max(1, shots);
}

bool is_visible(const vec3 &pos)
{
	if (!game->local_player || game->local_player->get_flash_bang_time() > game->globals->curtime)
		return false;

	const auto eye = game->local_player->get_eye_position();
	// TODO:
	// static const auto line_goes_through_smoke = PATTERN(uint32_t, "client.dll", "55 8B EC 83 EC 08 8B 15 ? ? ? ?
	// 0F"); if (cfg.legit.hack.smokecheck.get() && reinterpret_cast<bool(__cdecl*)(vec3,
	// vec3)>(line_goes_through_smoke())(eye, pos)) 	return false;

	trace_no_player_filter filter{};
	game_trace t;
	ray r;
	r.init(eye, pos);
	game->engine_trace->trace_ray(r, mask_visible | contents_blocklos, &filter, &t);
	return t.fraction == 1.f;
}

std::pair<float, float> perform_freestanding(const vec3 &from, std::vector<sdk::vec3> tos, bool *previous)
{
	if (tos.empty())
		INVALID_ARGUMENT("Target vector was empty");

	// setup data for tracing.
	const auto info = *game->weapon_system->get_weapon_data(item_definition_index::weapon_awp);
	float real{}, fake{}, current_back{};
	auto is_forced = false;

	for (auto &to : tos)
	{
		// calculate distance and angle to target.
		const auto at_target = calc_angle(from, to);
		const auto dist = (to - from).length();

		// directions of local player.
		const auto direction_left = std::remainderf(at_target.y - 90.f, yaw_bounds);
		const auto direction_right = std::remainderf(at_target.y + 90.f, yaw_bounds);
		const auto direction_back = std::remainderf(at_target.y + 180.f, yaw_bounds);

		// calculate scan positions.
		const auto local_left = rotate_2d(from, direction_left, freestand_width);
		const auto local_right = rotate_2d(from, direction_right, freestand_width);
		const auto local_half_left = rotate_2d(from, direction_left, .5f * freestand_width);
		const auto local_half_right = rotate_2d(from, direction_right, .5f * freestand_width);
		const auto enemy_left = rotate_2d(to, direction_left, freestand_width);
		const auto enemy_right = rotate_2d(to, direction_right, freestand_width);

		// comperator function.
		const auto compare = [&](const vec3 &fleft, const vec3 &fright, const vec3 &l, const vec3 &r,
								 const bool check = false, const bool check2 = false) -> float
		{
			auto cinfo = info;
			if (check)
				cinfo.idamage = 200;

			const auto res_left = trace.wall_penetration(fleft, l, nullptr, custom_tracing::bullet_safety::none,
														 std::nullopt, nullptr, false, &cinfo);
			if (check && res_left.damage > 0 && res_left.impact_count > 0 &&
				(res_left.impacts.front() - fleft).length() < (res_left.impacts.front() - l).length())
				return FLT_MAX;

			if (check2)
				return res_left.damage > 0 ? FLT_MAX : direction_back;

			const auto res_right = trace.wall_penetration(fright, r, nullptr, custom_tracing::bullet_safety::none,
														  std::nullopt, nullptr, false, &cinfo);
			if (check)
			{
				if (res_right.damage > 0 && res_right.impact_count > 0 &&
					(res_right.impacts.front() - fright).length() < (res_right.impacts.front() - r).length())
					return FLT_MAX;
			}
			else
			{
				if (res_right.damage && !res_left.damage)
					return direction_left;
				if (!res_right.damage && res_left.damage)
					return direction_right;
				if (res_right.damage && res_left.damage)
					return FLT_MAX;
			}

			return direction_back;
		};

		if (!is_forced)
		{
			if (const auto r1 = compare(to, to, local_left, local_right); r1 != direction_back)
			{
				if (r1 != FLT_MAX)
				{
					real = fake = r1;
					current_back = direction_back;
				}
				else
					real = fake = current_back = direction_back;

				is_forced = true;
			}
			else if (const auto r2 = compare(enemy_left, enemy_right, local_left, local_right); r2 != direction_back)
			{
				if (r2 != FLT_MAX)
				{
					real = fake = r2;
					current_back = direction_back;
				}
				else
					real = fake = current_back = direction_back;

				is_forced = true;
			}
		}

		if (real != direction_back && compare(to, {}, real == direction_left ? local_half_left : local_half_right, {},
											  false, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}
		if (real != direction_back &&
			compare(real == direction_left ? enemy_right : enemy_left, {},
					real == direction_left ? local_half_left : local_half_right, {}, false, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}
		if (real != direction_back &&
			compare(enemy_left, enemy_right, local_half_right, local_half_left, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}

		if (previous)
		{
			if (real == direction_back)
				real = *previous ? direction_left : direction_right;
			*previous = real == direction_left;
			break;
		}
	}

	const auto force_back = real != fake;
	if (!previous && (force_back || !is_forced))
	{
		vec3 sum;
		for (auto &to : tos)
			sum += to;
		sum /= tos.size();
		real = calc_angle(sum, from).y;
	}

	if (!force_back && cfg.antiaim.yaw.get().test(cfg_t::yaw_freestanding))
		fake = current_back;

	return std::make_pair(real, fake);
}
} // namespace aim_helper
} // namespace detail
