#include <base/cfg.h>
#include <lua/api_def.h>
#include <lua/helpers.h>
#include <sdk/convar.h>

using namespace lua;

int api_def::cvar::index(lua_State *l)
{
	runtime_state s(l);

	const auto cv = game->cvar->find_var(s.get_string(2));
	if (!cv)
		return 0;

	if (cv->flags & FCVAR_CHEAT || cv->flags & FCVAR_DEVELOPMENTONLY)
	{
		if (!cfg.lua.allow_insecure.get())
		{
			s.error(XOR_STR("cannot access cheat or dev only convars with Allow insecure disabled"));
			return 0;
		}
	}

	const auto is_blocked = std::find(helpers::blocked_vars.begin(), helpers::blocked_vars.end(),
									  util::fnv1a(cv->name)) != helpers::blocked_vars.end();
	const auto is_risky = std::find(helpers::risky_vars.begin(), helpers::risky_vars.end(), util::fnv1a(cv->name)) !=
						  helpers::risky_vars.end();
	if (is_blocked || is_risky)
	{
		if (!cfg.lua.allow_insecure.get())
		{
			s.error(XOR_STR("convar is blocked from access with Allow insecure disabled"));
			return 0;
		}
	}

	s.create_user_object_ptr(XOR_STR("csgo.cvar"), cv);
	return 1;
}

int api_def::cvar::get_int(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
	{
		s.error(XOR_STR("usage: obj:get_int()"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);
	s.push(cv->get_int());

	return 1;
}

int api_def::cvar::get_float(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
	{
		s.error(XOR_STR("usage: obj:get_float()"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);
	s.push(cv->get_float());
	return 1;
}

int api_def::cvar::set_int(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: obj:set_int(v)"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);

	const auto is_blocked = std::find(helpers::blocked_vars.begin(), helpers::blocked_vars.end(),
									  util::fnv1a(cv->name)) != helpers::blocked_vars.end();
	if (is_blocked)
	{
		s.error(XOR_STR("convar is blocked from changes"));
		return 0;
	}

	cv->set_int(s.get_integer(2));
	return 0;
}

int api_def::cvar::set_float(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: obj:set_float(v)"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);

	const auto is_blocked = std::find(helpers::blocked_vars.begin(), helpers::blocked_vars.end(),
									  util::fnv1a(cv->name)) != helpers::blocked_vars.end();
	if (is_blocked)
	{
		s.error(XOR_STR("convar is blocked from changes"));
		return 0;
	}

	cv->set_float(s.get_float(2));
	return 0;
}

int api_def::cvar::get_string(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
	{
		s.error(XOR_STR("usage: obj:get_string()"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);
	s.push(cv->value.string);
	return 1;
}

int api_def::cvar::set_string(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: obj:set_string(v)"));
		return 0;
	}

	const auto cv = s.user_data_ptr<sdk::convar>(1);

	const auto is_blocked = std::find(helpers::blocked_vars.begin(), helpers::blocked_vars.end(),
									  util::fnv1a(cv->name)) != helpers::blocked_vars.end();
	if (is_blocked)
	{
		s.error(XOR_STR("convar is blocked from changes"));
		return 0;
	}

	cv->set_string(s.get_string(2));
	return 0;
}