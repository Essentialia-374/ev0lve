
#include <base/hook_manager.h>
#include <detail/dx_adapter.h>
#include <menu/menu.h>
#include <sdk/global_vars_base.h>
#include <sdk/input_system.h>

#include <menu/effects/snow_effect.h>
#include <ren/adapters/adapter_dx9.h>
#include <ren/renderer.h>
#include <ren/types/animated_texture.h>

#include <base/debug_overlay.h>
#include <detail/entity_rendering.h>

#include <network/helpers.h>
#include <network/load_indicator.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif
#include <fonts/fonts.h>

using namespace evo;
using namespace ren;
using namespace hooks;
using namespace menu;
using namespace detail;

struct dx_backup
{
	explicit dx_backup(IDirect3DDevice9 *device)
	{
		device->CreateStateBlock(D3DSBT_PIXELSTATE, &pixel_state);
		pixel_state->Capture();

		device->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgb);
		device->GetRenderState(D3DRS_SRGBWRITEENABLE, &color);
		device->GetFVF(&fvf);

		device->GetStreamSource(0, &stream_data, &stream_offset, &stream_stride);

		device->GetTexture(0, &texture);
		device->GetRenderTarget(0, &target);
		device->GetIndices(&indices);
		device->GetVertexDeclaration(&vertex_declaration);
		device->GetPixelShader(&pixel_shader);
		device->GetVertexShader(&vertex_shader);
	}

	~dx_backup()
	{
		if (stream_data)
			stream_data->Release();
		if (texture)
			texture->Release();
		if (target)
			target->Release();
		if (indices)
			indices->Release();
		if (vertex_declaration)
			vertex_declaration->Release();
		if (pixel_shader)
			pixel_shader->Release();
		if (vertex_shader)
			vertex_shader->Release();
		if (pixel_state)
			pixel_state->Release();
	}

	void apply(IDirect3DDevice9 *device)
	{
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, color);
		device->SetFVF(fvf);

		device->SetStreamSource(0, stream_data, stream_offset, stream_stride);

		device->SetTexture(0, texture);
		device->SetRenderTarget(0, target);
		device->SetIndices(indices);
		device->SetVertexDeclaration(vertex_declaration);
		device->SetPixelShader(pixel_shader);
		device->SetVertexShader(vertex_shader);
		pixel_state->Apply();
	}

	DWORD srgb{}, color{}, fvf{};
	IDirect3DVertexBuffer9 *stream_data = nullptr;
	UINT stream_offset{};
	UINT stream_stride{};
	IDirect3DBaseTexture9 *texture = nullptr;
	IDirect3DSurface9 *target = nullptr;
	IDirect3DIndexBuffer9 *indices = nullptr;
	IDirect3DVertexDeclaration9 *vertex_declaration = nullptr;
	IDirect3DPixelShader9 *pixel_shader = nullptr;
	IDirect3DVertexShader9 *vertex_shader = nullptr;
	IDirect3DStateBlock9 *pixel_state = nullptr;
};

HRESULT __stdcall steam::present(IDirect3DDevice9 *dev, RECT *a, RECT *b, HWND c, RGNDATA *d)
{
	dx_backup bak(dev);

	if (!draw.adapter)
	{
		draw.adapter = std::make_shared<adapter_dx9>(game->input_system->get_attached_window(), dev);
		draw.create_layer(dx_adapter::esp_layer);
		draw.create_layer(31);
		draw.create_layer(63);

		draw.adapter->on_vertex_shader_compilation_error = [](const char *err)
		{
			print_to_console(sdk::color::attention(), XOR_STR("Failed to compile vertex shader:\n"));
			print_to_console(err);
		};

		draw.adapter->on_fragment_shader_compilation_error = [](const char *err)
		{
			print_to_console(sdk::color::attention(), XOR_STR("Failed to compile fragment shader:\n"));
			print_to_console(err);
		};

		buf = std::make_shared<layer>(draw.adapter->alloc_vb(), draw.adapter->alloc_ib());
		buf->vb->should_discard = false;
		buf->ib->should_discard = false;
		buf->create();

		const auto &d = draw.layers[dx_adapter::esp_layer];
		d->vb->should_discard = false;
		d->ib->should_discard = false;

		evo::gui::ctx = std::make_unique<evo::gui::context>();

		char win_dir_arr[256]{};
		DWORD pixel;
		GetSystemWindowsDirectoryA(win_dir_arr, 256);
		draw.mem_font_hadles.push_back(AddFontMemResourceEx(SFUIDisplay_Bold, 181484, nullptr, &pixel));
		draw.mem_font_hadles.push_back(AddFontMemResourceEx(SFUIDisplay_Regular, 170404, nullptr, &pixel));
		draw.mem_font_hadles.push_back(AddFontMemResourceEx(SFUIDisplay_SemiBold, 193080, nullptr, &pixel));


		std::string win_dir{win_dir_arr};
		draw.fonts[FNV1A("tahoma8")] =
			std::make_shared<font_gdi>(XOR_STR("Smallest Pixel-7"), 11.f, font_flag_outline, 0, 0x80);
		draw.fonts[FNV1A("tahoma13")] =
			std::make_shared<font>(
				(win_dir + ("/fonts/segoeuib.ttf")).c_str(), 13.f, font_flag_shadow | font_flag_no_dpi, 0, 0x45F);
		draw.fonts[FNV1A("segoeuib28")] =
			std::make_shared<font>((win_dir + XOR("/fonts/segoeuib.ttf")).c_str(), 28.f, font_flag_shadow, 0, 0x80); //segoeuib.ttf

		menu::menu.create();
		//network::load_required_resources();
		draw.refresh();
		draw.create_objects();


		network::load_ind.begin();
	}

	draw.frame_time = game->globals->frametime;
	draw.time = game->globals->realtime;
	draw.refresh();

	dx_adapter::mtx.lock();
	if (dx_adapter::ready)
	{
		const auto &d = draw.layers[dx_adapter::esp_layer];
		d->ignore_flush = false;
		d->reset(true);

		dx_adapter::swap_buffers();
		dx_adapter::ready = false;
	}
	dx_adapter::mtx.unlock();

	draw.begin_frame();

	if (network::load_ind.did_begin)
		network::load_ind.render();

	evo::gui::ctx->render();

	draw.end_frame();
	draw.render();
	draw.flush();

	menu::menu.finalize();

	bak.apply(dev);
	return hook_manager.present->call(dev, a, b, c, d);
}

HRESULT __stdcall steam::reset(IDirect3DDevice9 *dev, D3DPRESENT_PARAMETERS *pp)
{
	if (!draw.adapter)
		return hook_manager.reset->call(dev, pp);

	draw.destroy_objects();
	dx_adapter::destroy_objects();

	const auto r = hook_manager.reset->call(dev, pp);

#if defined(CSGO_SECURE) && !defined(_DEBUG)
	if (const auto as = network::get_user_data().avatar_size; as)
	{
		auto dec_avatar = network::get_decoded_avatar();
		gui::ctx->user.avatar =
			std::make_shared<ren::animated_texture>(reinterpret_cast<void *>(dec_avatar.data()), as);
		gui::ctx->user.avatar->create();

		draw.textures[GUI_HASH("user_avatar")] = gui::ctx->user.avatar;
	}
#endif

	draw.refresh();
	draw.create_objects();
	dx_adapter::create_objects();
	return r;
}