
#ifndef SDK_MODEL_RENDER_H
#define SDK_MODEL_RENDER_H

#include <sdk/client_entity.h>
#include <sdk/color.h>
#include <sdk/cs_player.h>
#include <sdk/keyvalues.h>
#include <sdk/macros.h>
#include <sdk/mat.h>
#include <sdk/model_info_client.h>
#include <sdk/vec2.h>
#include <sdk/vec3.h>
#include <sdk/vec4.h>

class IDirect3DTexture9;

namespace sdk
{
inline constexpr auto studio_render = 1;

enum material_primitive_type
{
	material_points,
	material_lines,
	material_triangles,
	material_triangle_strip,
	material_line_strip,
	material_line_loop,
	material_polygon,
	material_quads,
	material_subd_quads_extra,
	material_subd_quads_reg,
	material_instanced_quads,
	material_heterogenous
};

struct model
{
	uintptr_t pad;
	char name[255];
};

struct model_render_info_t
{
	vec3 origin;
	angle angles;
	char pad[0x4];
	client_renderable *renderable;
	const model *model;
	const mat3x4 *model_to_world;
	const mat3x4 *lighting_offset;
	const vec3 *lighting_origin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	uint32_t instance;
};

class material_var
{
	VIRTUAL(10, set_vec3_internal(const float x, const float y), void(__thiscall *)(void *, float, float))(x, y);
	VIRTUAL(11, set_vec3_internal(const float x, const float y, const float z),
			void(__thiscall *)(void *, float, float, float))
	(x, y, z);

public:
	VIRTUAL(4, set_float(const float val), void(__thiscall *)(void *, float))(val);
	VIRTUAL(5, set_int(const int val), void(__thiscall *)(void *, int))(val);
	VIRTUAL(6, set_string(const char *val), void(__thiscall *)(void *, const char *))(val);
	VIRTUAL(7, get_string(), const char *(__thiscall *)(void *))();
	VIRTUAL(20, set_matrix(viewmat &matrix), void(__thiscall *)(void *, viewmat &))(matrix);
	VIRTUAL(26, set_vec3_component(const float val, const int comp), void(__thiscall *)(void *, float, int))
	(val, comp);
	VIRTUAL(27, get_int(), int(__thiscall *)(void *))();
	VIRTUAL(28, get_float(), float(__thiscall *)(void *))();
	VIRTUAL(29, get_vec3(), float *(__thiscall *)(void *))();
	VIRTUAL(31, get_vec3_size(), int(__thiscall *)(void *))();

	void set_vec3(const vec2 vec3) { set_vec3_internal(vec3.x, vec3.y); }

	void set_vec3(const vec3 vec3) { set_vec3_internal(vec3.x, vec3.y, vec3.z); }
};

enum material_var_flags
{
	material_var_debug = 1 << 0,
	material_var_no_debug_override = 1 << 1,
	material_var_no_draw = 1 << 2,
	material_var_use_in_fillrate_mode = 1 << 3,
	material_var_vertexcolor = 1 << 4,
	material_var_vertexalpha = 1 << 5,
	material_var_selfillum = 1 << 6,
	material_var_additive = 1 << 7,
	material_var_alphatest = 1 << 8,
	material_var_znearer = 1 << 10,
	material_var_model = 1 << 11,
	material_var_flat = 1 << 12,
	material_var_nocull = 1 << 13,
	material_var_nofog = 1 << 14,
	material_var_ignorez = 1 << 15,
	material_var_decal = 1 << 16,
	material_var_envmapsphere = 1 << 17,
	material_var_envmapcameraspace = 1 << 19,
	material_var_basealphaenvmapmask = 1 << 20,
	material_var_translucent = 1 << 21,
	material_var_normalmapalphaenvmapmask = 1 << 22,
	material_var_needs_software_skinning = 1 << 23,
	material_var_opaquetexture = 1 << 24,
	material_var_envmapmode = 1 << 25,
	material_var_suppress_decals = 1 << 26,
	material_var_halflambert = 1 << 27,
	material_var_wireframe = 1 << 28,
	material_var_allowalphatocoverage = 1 << 29,
	material_var_alpha_modified_by_proxy = 1 << 30,
	material_var_vertexfog = 1 << 31
};

struct renderable_info
{
	client_renderable *renderable;
	uintptr_t alpha_prop;
	int32_t enum_count;
	int32_t render_frame;
	uint16_t first_shadow;
	uint16_t first_leaf;
	int16_t area;
	uint16_t flags;
	uint16_t render_fast_reflection : 1;
	uint16_t disable_shadow_depth_rendering : 1;
	uint16_t disable_csm_rendering : 1;
	uint16_t disable_shadow_depth_caching : 1;
	uint16_t split_screen_enabled : 2;
	uint16_t translucency_type : 2;
	uint16_t model_type : 8;
	vec3 bloated_abs_mins;
	vec3 bloated_abs_maxs;
	vec3 abs_mins;
	vec3 abs_maxs;
	int32_t pad;
};

struct texture_handle_t
{
	char pad[0xC];
	IDirect3DTexture9 *texture;
};

#define MAX_CASCADED_SHADOW_MAPPING_CASCADES (4)

// The number of float4's in the CascadedShadowMappingState_t (starting at m_vLightColor)
#define CASCADED_SHADOW_MAPPING_CONSTANT_BUFFER_SIZE (26)

// Note: This struct is sent as-is as an array of pixel shader constants (starting at m_vLightColor). If you modify it, be sure to update CASCADED_SHADOW_MAPPING_CONSTANT_BUFFER_SIZE.
struct CascadedShadowMappingState_t
{
	uint32_t m_nNumCascades;
	bool m_bIsRenderingViewModels;

	vec4 m_vLightColor;

	vec3 m_vLightDir;
	float m_flPadding1;

	struct
	{
		float m_flInvShadowTextureWidth;
		float m_flInvShadowTextureHeight;
		float m_flHalfInvShadowTextureWidth;
		float m_flHalfInvShadowTextureHeight;
	} m_TexParams;

	struct
	{
		float m_flShadowTextureWidth;
		float m_flShadowTextureHeight;
		float m_flSplitLerpFactorBase;
		float m_flSplitLerpFactorInvRange;
	} m_TexParams2;

	struct
	{
		float m_flDistLerpFactorBase;
		float m_flDistLerpFactorInvRange;
		float m_flUnused0;
		float m_flUnused1;
	} m_TexParams3;

	vmatrix m_matWorldToShadowTexMatrices[MAX_CASCADED_SHADOW_MAPPING_CASCADES];
	vec4 m_vCascadeAtlasUVOffsets[MAX_CASCADED_SHADOW_MAPPING_CASCADES];

	vec4 m_vCamPosition;
};

enum
{
	MATERIAL_MAX_LIGHT_COUNT = 4,
};

enum light_type_t
{
	MATERIAL_LIGHT_DISABLE = 0,
	MATERIAL_LIGHT_POINT,
	MATERIAL_LIGHT_DIRECTIONAL,
	MATERIAL_LIGHT_SPOT,
};

struct light_desc_t
{
	light_type_t m_Type;
	vec3 m_Color;
	vec3 m_Position;
	vec3 m_Direction;
	float  m_Range;
	float m_Falloff;
	float m_Attenuation0;
	float m_Attenuation1;
	float m_Attenuation2;
	float m_Theta;
	float m_Phi;
	float m_ThetaDot;
	float m_PhiDot;
	unsigned int m_Flags;
	float OneOver_ThetaDot_Minus_PhiDot;
	float m_RangeSquared;
};

struct MaterialLightingState_t
{
	vec3			m_vecAmbientCube[6];		// ambient, and lights that aren't in locallight[]
	vec3			m_vecLightingOrigin;		// The position from which lighting state was computed
	int				m_nLocalLightCount;
	light_desc_t		m_pLocalLightDesc[MATERIAL_MAX_LIGHT_COUNT];

	MaterialLightingState_t& operator=(const MaterialLightingState_t& src)
	{
		memcpy(this, &src, sizeof(MaterialLightingState_t) - MATERIAL_MAX_LIGHT_COUNT * sizeof(light_desc_t));
		size_t byteCount = src.m_nLocalLightCount * sizeof(light_desc_t);
		assert(byteCount <= sizeof(m_pLocalLightDesc));
		memcpy(m_pLocalLightDesc, &src.m_pLocalLightDesc, byteCount);
		return *this;
	}
};


class texture
{
public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
	VIRTUAL(3, get_actual_width(), int32_t(__thiscall *)(void *))();
	VIRTUAL(4, get_actual_height(), int32_t(__thiscall *)(void *))();
	VIRTUAL(10, increment_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(11, decrement_reference_count(), void(__thiscall *)(void *))();

	char pad[0x50];
	texture_handle_t **handles;
};

class material;


struct stencil_state
{
	bool enable = true;
	uint32_t fail = 1;
	uint32_t zfail = 3;
	uint32_t pass = 1;
	uint32_t cmp = 8;
	int32_t reference = 1;
	uint32_t test = 0xFFFFFFFF;
	uint32_t write = 0xFFFFFFFF;
};



typedef uint64_t vertex_format_t;
typedef uint32_t morph_format_t;
using occlusion_query_object_handle_t = void*;



struct morph_weight_t;
struct material_primitive_type_t;
struct material_non_interactive_mode_t;
struct material_matrix_mode_t;
struct material_index_format_t;
struct i_mesh;
enum material_height_clip_mode_t
{
	MATERIAL_HEIGHTCLIPMODE_DISABLE,
	MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT,
	MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT
};
struct material_fog_mode_t;
enum material_cull_mode_t
{
	MATERIAL_CULLMODE_CCW,	// this culls polygons with counterclockwise winding
	MATERIAL_CULLMODE_CW	// this culls polygons with clockwise winding
};

struct i_vertex_buffer;
struct i_morph;
struct i_index_buffer;
struct i_call_queue;
struct i_async_texture_operation_receiver;
struct flashlight_state_t;
struct deformation_base_t;
struct color_correction_handle_t;

enum stencil_operation_t
{
	STENCILOPERATION_KEEP = 1,
	STENCILOPERATION_ZERO = 2,
	STENCILOPERATION_REPLACE = 3,
	STENCILOPERATION_INCRSAT = 4,
	STENCILOPERATION_DECRSAT = 5,
	STENCILOPERATION_INVERT = 6,
	STENCILOPERATION_INCR = 7,
	STENCILOPERATION_DECR = 8,
	STENCILOPERATION_FORCE_DWORD = 0x7fffffff
};

enum stencil_comparison_function_t
{
	STENCILCOMPARISONFUNCTION_NEVER = 1,
	STENCILCOMPARISONFUNCTION_LESS = 2,
	STENCILCOMPARISONFUNCTION_EQUAL = 3,
	STENCILCOMPARISONFUNCTION_LESSEQUAL = 4,
	STENCILCOMPARISONFUNCTION_GREATER = 5,
	STENCILCOMPARISONFUNCTION_NOTEQUAL = 6,
	STENCILCOMPARISONFUNCTION_GREATEREQUAL = 7,
	STENCILCOMPARISONFUNCTION_ALWAYS = 8,
	STENCILCOMPARISONFUNCTION_FORCE_DWORD = 0x7fffffff
};


class i_ref_counted
{
public:
	virtual int add_ref() = 0;
	virtual int release() = 0;
};

struct MaterialMatrixMode_t;
struct MaterialCullMode_t;
struct MaterialFogMode_t;
struct MaterialHeightClipMode_t;

class IVertexBuffer;
class IIndexBuffer;
struct OcclusionQueryObjectHandle_t;
class IMesh;
struct FlashlightState_t;
struct DeformationBase_t;
struct ColorCorrectionHandle_t;
struct MaterialNonInteractiveMode_t;
enum MaterialIndexFormat_t
{
	MATERIAL_INDEX_FORMAT_UNKNOWN = -1,
	MATERIAL_INDEX_FORMAT_16BIT = 0,
	MATERIAL_INDEX_FORMAT_32BIT,
};
class ICallQueue;
struct mesh_instance_data
{
	int32_t index_offset;
	int32_t index_count;
	int32_t bone_count;
	uintptr_t bone_remap;
	mat3x4* pose;
	texture* env_cubemap;
	uintptr_t lighting_state;
	material_primitive_type prim_type;
	uintptr_t vertex;
	int32_t vertex_offset;
	uintptr_t index;
	uintptr_t color;
	int32_t color_offset;
	uintptr_t stencil_state;
	quaternion diffuse_modulation;
	int32_t lightmap_page_id;
	bool indirect_lighting_only;
};
class mat_render_context : public i_ref_counted
{
public:
	virtual void				BeginRender() = 0;
	virtual void				EndRender() = 0;

	virtual void				Flush(bool flushHardware = false) = 0;

	virtual void				BindLocalCubemap(texture* pTexture) = 0;

	// pass in an texture (that is build with "rendertarget" "1") or
	// pass in NULL for the regular backbuffer.
	virtual void				SetRenderTarget(texture* pTexture) = 0;
	virtual texture* GetRenderTarget(void) = 0;

	virtual void				GetRenderTargetDimensions(int& width, int& height) const = 0;

	// Bind a material is current for rendering.
	virtual void				Bind(material* material, void* proxyData = 0) = 0;
	// Bind a lightmap page current for rendering.  You only have to
	// do this for materials that require lightmaps.
	virtual void				BindLightmapPage(int lightmapPageID) = 0;

	// inputs are between 0 and 1
	virtual void				DepthRange(float zNear, float zFar) = 0;

	virtual void				ClearBuffers(bool bClearColor, bool bClearDepth, bool bClearStencil = false) = 0;

	// read to a unsigned char rgb image.
	virtual void				ReadPixels(int x, int y, int width, int height, unsigned char* data, e_image_format dstFormat, texture* pRenderTargetTexture = NULL) = 0;
	virtual void				ReadPixelsAsync(int x, int y, int width, int height, unsigned char* data, e_image_format dstFormat, texture* pRenderTargetTexture = NULL, void* pPixelsReadEvent = NULL) = 0;
	virtual void				ReadPixelsAsyncGetResult(int x, int y, int width, int height, unsigned char* data, e_image_format dstFormat, void* pGetResultEvent = NULL) = 0;

	// Sets lighting
	virtual void				SetLightingState(const MaterialLightingState_t& state) = 0;
	virtual void				SetLights(int nCount, const light_desc_t* pLights) = 0;

	// The faces of the cube are specified in the same order as cubemap textures
	virtual void				SetAmbientLightCube(vec4 cube[6]) = 0;

	// Blit the backbuffer to the framebuffer texture
	virtual void				CopyRenderTargetToTexture(texture* pTexture) = 0;

	// Set the current texture that is a copy of the framebuffer.
	virtual void				SetFrameBufferCopyTexture(texture* pTexture, int textureIndex = 0) = 0;
	virtual texture* GetFrameBufferCopyTexture(int textureIndex) = 0;

	//
	// end vertex array api
	//

	// matrix api
	virtual void				MatrixMode(MaterialMatrixMode_t matrixMode) = 0;
	virtual void				PushMatrix(void) = 0;
	virtual void				PopMatrix(void) = 0;
	virtual void				LoadMatrix(vmatrix const& matrix) = 0;
	virtual void				LoadMatrix(mat3x4 const& matrix) = 0;
	virtual void				MultMatrix(vmatrix const& matrix) = 0;
	virtual void				MultMatrix(mat3x4 const& matrix) = 0;
	virtual void				MultMatrixLocal(vmatrix const& matrix) = 0;
	virtual void				MultMatrixLocal(mat3x4 const& matrix) = 0;
	virtual void				GetMatrix(MaterialMatrixMode_t matrixMode, vmatrix* matrix) = 0;
	virtual void				GetMatrix(MaterialMatrixMode_t matrixMode, mat3x4* matrix) = 0;
	virtual void				LoadIdentity(void) = 0;
	virtual void				Ortho(double left, double top, double right, double bottom, double zNear, double zFar) = 0;
	virtual void				PerspectiveX(double fovx, double aspect, double zNear, double zFar) = 0;
	virtual void				PickMatrix(int x, int y, int width, int height) = 0;
	virtual void				Rotate(float angle, float x, float y, float z) = 0;
	virtual void				Translate(float x, float y, float z) = 0;
	virtual void				Scale(float x, float y, float z) = 0;
	// end matrix api

	// Sets/gets the viewport
	virtual void				Viewport(int x, int y, int width, int height) = 0;
	virtual void				GetViewport(int& x, int& y, int& width, int& height) const = 0;

	// The cull mode
	virtual void				CullMode(MaterialCullMode_t cullMode) = 0;
	virtual void				FlipCullMode(void) = 0; //CW->CCW or CCW->CW, intended for mirror support where the view matrix is flipped horizontally

	// Shadow buffer generation
	virtual void				BeginGeneratingCSMs() = 0;
	virtual void				EndGeneratingCSMs() = 0;
	virtual void				PerpareForCascadeDraw(int cascade, float fShadowSlopeScaleDepthBias, float fShadowDepthBias) = 0;

	// end matrix api

	// This could easily be extended to a general user clip plane
	virtual void				SetHeightClipMode(MaterialHeightClipMode_t nHeightClipMode) = 0;
	// garymcthack : fog z is always used for heightclipz for now.
	virtual void				SetHeightClipZ(float z) = 0;

	// Fog methods...
	virtual void				FogMode(MaterialFogMode_t fogMode) = 0;
	virtual void				FogStart(float fStart) = 0;
	virtual void				FogEnd(float fEnd) = 0;
	virtual void				SetFogZ(float fogZ) = 0;
	virtual MaterialFogMode_t	GetFogMode(void) = 0;

	virtual void				FogColor3f(float r, float g, float b) = 0;
	virtual void				FogColor3fv(float const* rgb) = 0;
	virtual void				FogColor3ub(unsigned char r, unsigned char g, unsigned char b) = 0;
	virtual void				FogColor3ubv(unsigned char const* rgb) = 0;

	virtual void				GetFogColor(unsigned char* rgb) = 0;

	// Sets the number of bones for skinning
	virtual void				SetNumBoneWeights(int numBones) = 0;

	// Creates/destroys Mesh
	virtual IMesh* CreateStaticMesh(VertexFormat_t fmt, const char* pTextureBudgetGroup, material* pMaterial = NULL, VertexStreamSpec_t* pStreamSpec = NULL) = 0;
	virtual void DestroyStaticMesh(IMesh* mesh) = 0;

	// Gets the dynamic mesh associated with the currently bound material
	// note that you've got to render the mesh before calling this function
	// a second time. Clients should *not* call DestroyStaticMesh on the mesh
	// returned by this call.
	// Use buffered = false if you want to not have the mesh be buffered,
	// but use it instead in the following pattern:
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		Draw partial
	//		Draw partial
	//		Draw partial
	//		meshBuilder.Begin
	//		meshBuilder.End
	//		etc
	// Use Vertex or Index Override to supply a static vertex or index buffer
	// to use in place of the dynamic buffers.
	//
	// If you pass in a material in pAutoBind, it will automatically bind the
	// material. This can be helpful since you must bind the material you're
	// going to use BEFORE calling GetDynamicMesh.
	virtual IMesh* GetDynamicMesh(
		bool buffered = true,
		IMesh* pVertexOverride = 0,
		IMesh* pIndexOverride = 0,
		material* pAutoBind = 0) = 0;

	// ------------ New Vertex/Index Buffer interface ----------------------------
	// Do we need support for bForceTempMesh and bSoftwareVertexShader?
	// I don't think we use bSoftwareVertexShader anymore. .need to look into bForceTempMesh.
	virtual IVertexBuffer* CreateStaticVertexBuffer(VertexFormat_t fmt, int nVertexCount, const char* pTextureBudgetGroup) = 0;
	virtual IIndexBuffer* CreateStaticIndexBuffer(MaterialIndexFormat_t fmt, int nIndexCount, const char* pTextureBudgetGroup) = 0;
	virtual void DestroyVertexBuffer(IVertexBuffer*) = 0;
	virtual void DestroyIndexBuffer(IIndexBuffer*) = 0;
	// Do we need to specify the stream here in the case of locking multiple dynamic VBs on different streams?
	virtual IVertexBuffer* GetDynamicVertexBuffer(int streamID, VertexFormat_t vertexFormat, bool bBuffered = true) = 0;
	virtual IIndexBuffer* GetDynamicIndexBuffer() = 0;
	virtual void BindVertexBuffer(int streamID, IVertexBuffer* pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions = 1) = 0;
	virtual void BindIndexBuffer(IIndexBuffer* pIndexBuffer, int nOffsetInBytes) = 0;
	virtual void Draw(MaterialPrimitiveType_t primitiveType, int firstIndex, int numIndices) = 0;
	// ------------ End ----------------------------

	// Selection mode methods
	virtual int  SelectionMode(bool selectionMode) = 0;
	virtual void SelectionBuffer(unsigned int* pBuffer, int size) = 0;
	virtual void ClearSelectionNames() = 0;
	virtual void LoadSelectionName(int name) = 0;
	virtual void PushSelectionName(int name) = 0;
	virtual void PopSelectionName() = 0;

	// Sets the Clear Color for ClearBuffer....
	virtual void		ClearColor3ub(unsigned char r, unsigned char g, unsigned char b) = 0;
	virtual void		ClearColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) = 0;

	// Allows us to override the depth buffer setting of a material
	virtual void	OverrideDepthEnable(bool bEnable, bool bDepthWriteEnable, bool bDepthTestEnable = true) = 0;

	// FIXME: This is a hack required for NVidia/XBox, can they fix in drivers?
	virtual void	DrawScreenSpaceQuad(material* pMaterial) = 0;

	// For debugging and building recording files. This will stuff a token into the recording file,
	// then someone doing a playback can watch for the token.
	virtual void	SyncToken(const char* pToken) = 0;

	// FIXME: REMOVE THIS FUNCTION!
	// The only reason why it's not gone is because we're a week from ship when I found the bug in it
	// and everything's tuned to use it.
	// It's returning values which are 2x too big (it's returning sphere diameter x2)
	// Use ComputePixelDiameterOfSphere below in all new code instead.
	virtual float	ComputePixelWidthOfSphere(const vec3& origin, float flRadius) = 0;

	//
	// Occlusion query support
	//

	// Allocate and delete query objects.
	virtual OcclusionQueryObjectHandle_t CreateOcclusionQueryObject(void) = 0;
	virtual void DestroyOcclusionQueryObject(OcclusionQueryObjectHandle_t) = 0;

	// Bracket drawing with begin and end so that we can get counts next frame.
	virtual void BeginOcclusionQueryDrawing(OcclusionQueryObjectHandle_t) = 0;
	virtual void EndOcclusionQueryDrawing(OcclusionQueryObjectHandle_t) = 0;

	// Get the number of pixels rendered between begin and end on an earlier frame.
	// Calling this in the same frame is a huge perf hit!
	virtual int OcclusionQuery_GetNumPixelsRendered(OcclusionQueryObjectHandle_t) = 0;

	virtual void SetFlashlightMode(bool bEnable) = 0;

	virtual void SetFlashlightState(const FlashlightState_t& state, const vmatrix& worldToTexture) = 0;

	virtual bool IsCascadedShadowMapping() const = 0;
	virtual void SetCascadedShadowMapping(bool bEnable) = 0;
	virtual void SetCascadedShadowMappingState(const CascadedShadowMappingState_t& state, texture* pDepthTextureAtlas) = 0;

	// Gets the current height clip mode
	virtual MaterialHeightClipMode_t GetHeightClipMode() = 0;

	// This returns the diameter of the sphere in pixels based on
	// the current model, view, + projection matrices and viewport.
	virtual float	ComputePixelDiameterOfSphere(const vec3& vecAbsOrigin, float flRadius) = 0;

	// By default, the material system applies the VIEW and PROJECTION matrices	to the user clip
	// planes (which are specified in world space) to generate projection-space user clip planes
	// Occasionally (for the particle system in hl2, for example), we want to override that
	// behavior and explictly specify a ViewProj transform for user clip planes
	virtual void	EnableUserClipTransformOverride(bool bEnable) = 0;
	virtual void	UserClipTransform(const vmatrix& worldToView) = 0;

	virtual bool GetFlashlightMode() const = 0;

	virtual bool IsCullingEnabledForSinglePassFlashlight() const = 0;
	virtual void EnableCullingForSinglePassFlashlight(bool bEnable) = 0;

	// Used to make the handle think it's never had a successful query before
	virtual void ResetOcclusionQueryObject(OcclusionQueryObjectHandle_t) = 0;

	// Creates/destroys morph data associated w/ a particular material
	virtual i_morph* CreateMorph(morph_format_t format, const char* pDebugName) = 0;
	virtual void DestroyMorph(i_morph* pMorph) = 0;

	// Binds the morph data for use in rendering
	virtual void BindMorph(i_morph* pMorph) = 0;

	// Sets flexweights for rendering
	virtual void SetFlexWeights(int nFirstWeight, int nCount, const morph_weight_t* pWeights) = 0;

	// Allocates temp render data. Renderdata goes out of scope at frame end in multicore
	// Renderdata goes out of scope after refcount goes to zero in singlecore.
	// Locking/unlocking increases + decreases refcount
	virtual void* LockRenderData(int nSizeInBytes) = 0;
	virtual void			UnlockRenderData(void* pData) = 0;

	// Typed version. If specified, pSrcData is copied into the locked memory.
	template< class E > E* LockRenderDataTyped(int nCount, const E* pSrcData = NULL);

	// Temp render data gets immediately freed after it's all unlocked in single core.
	// This prevents it from being freed
	virtual void			AddRefRenderData() = 0;
	virtual void			ReleaseRenderData() = 0;

	// Returns whether a pointer is render data. NOTE: passing NULL returns true
	virtual bool			IsRenderData(const void* pData) const = 0;

	// Read w/ stretch to a host-memory buffer
	virtual void ReadPixelsAndStretch(rect_t* pSrcRect, rect_t* pDstRect, unsigned char* pBuffer, e_image_format dstFormat, int nDstStride) = 0;

	// Gets the window size
	virtual void GetWindowSize(int& width, int& height) const = 0;

	virtual void				pad() = 0;

	// This function performs a texture map from one texture map to the render destination, doing
	// all the necessary pixel/texel coordinate fix ups. fractional values can be used for the
	// src_texture coordinates to get linear sampling - integer values should produce 1:1 mappings
	// for non-scaled operations.
	virtual void DrawScreenSpaceRectangle(
		material* pMaterial,
		int destx, int desty,
		int width, int height,
		float src_texture_x0, float src_texture_y0,			// which texel you want to appear at
		// destx/y
		float src_texture_x1, float src_texture_y1,			// which texel you want to appear at
		// destx+width-1, desty+height-1
		int src_texture_width, int src_texture_height,		// needed for fixup
		void* pClientRenderable = NULL,
		int nXDice = 1,
		int nYDice = 1) = 0;

	virtual void LoadBoneMatrix(int boneIndex, const mat3x4& matrix) = 0;

	// This version will push the current rendertarget + current viewport onto the stack
	virtual void PushRenderTargetAndViewport() = 0;

	// This version will push a new rendertarget + a maximal viewport for that rendertarget onto the stack
	virtual void PushRenderTargetAndViewport(texture* pTexture) = 0;

	// This version will push a new rendertarget + a specified viewport onto the stack
	virtual void PushRenderTargetAndViewport(texture* pTexture, int nViewX, int nViewY, int nViewW, int nViewH) = 0;

	// This version will push a new rendertarget + a specified viewport onto the stack
	virtual void PushRenderTargetAndViewport(texture* pTexture, texture* pDepthTexture, int nViewX, int nViewY, int nViewW, int nViewH) = 0;

	// This will pop a rendertarget + viewport
	virtual void PopRenderTargetAndViewport(void) = 0;

	// Binds a particular texture as the current lightmap
	virtual void BindLightmapTexture(texture* pLightmapTexture) = 0;

	// Blit a subrect of the current render target to another texture
	virtual void CopyRenderTargetToTextureEx(texture* pTexture, int nRenderTargetID, rect_t* pSrcRect, rect_t* pDstRect = NULL) = 0;
	virtual void CopyTextureToRenderTargetEx(int nRenderTargetID, texture* pTexture, rect_t* pSrcRect, rect_t* pDstRect = NULL) = 0;

	// Special off-center perspective matrix for DoF, MSAA jitter and poster rendering
	virtual void PerspectiveOffCenterX(double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right) = 0;

	// Rendering parameters control special drawing modes withing the material system, shader
	// system, shaders, and engine. renderparm.h has their definitions.
	virtual void SetFloatRenderingParameter(int parm_number, float value) = 0;
	virtual void SetIntRenderingParameter(int parm_number, int value) = 0;
	virtual void Setvec3RenderingParameter(int parm_number, vec3 const& value) = 0;

	// stencil buffer operations.
	virtual void SetStencilState(const stencil_state& state) = 0;
	virtual void ClearStencilBufferRectangle(int xmin, int ymin, int xmax, int ymax, int value) = 0;

	virtual void SetRenderTargetEx(int nRenderTargetID, texture* pTexture) = 0;

	// rendering clip planes, beware that only the most recently pushed plane will actually be used in a sizeable chunk of hardware configurations
	// and that changes to the clip planes mid-frame while UsingFastClipping() is true will result unresolvable depth inconsistencies
	virtual void PushCustomClipPlane(const float* pPlane) = 0;
	virtual void PopCustomClipPlane(void) = 0;

	// Returns the number of vertices + indices we can render using the dynamic mesh
	// Passing true in the second parameter will return the max # of vertices + indices
	// we can use before a flush is provoked and may return different values
	// if called multiple times in succession.
	// Passing false into the second parameter will return
	// the maximum possible vertices + indices that can be rendered in a single batch
	virtual void GetMaxToRender(IMesh* pMesh, bool bMaxUntilFlush, int* pMaxVerts, int* pMaxIndices) = 0;

	// Returns the max possible vertices + indices to render in a single draw call
	virtual int GetMaxVerticesToRender(material* pMaterial) = 0;
	virtual int GetMaxIndicesToRender() = 0;
	virtual void DisableAllLocalLights() = 0;
	virtual int CompareMaterialCombos(material* pMaterial1, material* pMaterial2, int lightMapID1, int lightMapID2) = 0;

	virtual IMesh* GetFlexMesh() = 0;

	virtual void SetFlashlightStateEx(const FlashlightState_t& state, const vmatrix& worldToTexture, texture* pFlashlightDepthTexture) = 0;

	// Returns the currently bound local cubemap
	virtual texture* GetLocalCubemap() = 0;

	// This is a version of clear buffers which will only clear the buffer at pixels which pass the stencil test
	virtual void ClearBuffersObeyStencil(bool bClearColor, bool bClearDepth) = 0;

	//enables/disables all entered clipping planes, returns the input from the last time it was called.
	virtual bool EnableClipping(bool bEnable) = 0;

	//get fog distances entered with FogStart(), FogEnd(), and SetFogZ()
	virtual void GetFogDistances(float* fStart, float* fEnd, float* fFogZ) = 0;

	// Hooks for firing PIX events from outside the Material System...
	virtual void BeginPIXEvent(unsigned long color, const char* szName) = 0;
	virtual void EndPIXEvent() = 0;
	virtual void SetPIXMarker(unsigned long color, const char* szName) = 0;

	// Batch API
	// from changelist 166623:
	// - replaced obtuse material system batch usage with an explicit and easier to thread API
	virtual void BeginBatch(IMesh* pIndices) = 0;
	virtual void BindBatch(IMesh* pVertices, material* pAutoBind = NULL) = 0;
	virtual void DrawBatch(MaterialPrimitiveType_t primType, int firstIndex, int numIndices) = 0;
	virtual void EndBatch() = 0;

	// Raw access to the call queue, which can be NULL if not in a queued mode
	virtual ICallQueue* GetCallQueue() = 0;

	// Returns the world-space camera position
	virtual void GetWorldSpaceCameraPosition(vec3* pCameraPos) = 0;
	virtual void GetWorldSpaceCameravec3s(vec3* pVecForward, vec3* pVecRight, vec3* pVecUp) = 0;

	// Set a linear vec3 color scale for all 3D rendering.
	// A value of [1.0f, 1.0f, 1.0f] should match non-tone-mapped rendering.

	virtual void				SetToneMappingScaleLinear(const vec3& scale) = 0;
	virtual vec3				GetToneMappingScaleLinear(void) = 0;

	virtual void				SetShadowDepthBiasFactors(float fSlopeScaleDepthBias, float fDepthBias) = 0;

	// Apply stencil operations to every pixel on the screen without disturbing depth or color buffers
	virtual void				PerformFullScreenStencilOperation(void) = 0;

	// Sets lighting origin for the current model (needed to convert directional lights to points)
	virtual void				SetLightingOrigin(vec3 vLightingOrigin) = 0;

	// Set scissor rect for rendering
	virtual void				PushScissorRect(const int nLeft, const int nTop, const int nRight, const int nBottom) = 0;
	virtual void				PopScissorRect() = 0;

	// Methods used to build the morph accumulator that is read from when HW morphing is enabled.
	virtual void				BeginMorphAccumulation() = 0;
	virtual void				EndMorphAccumulation() = 0;
	virtual void				AccumulateMorph(i_morph* pMorph, int nMorphCount, const morph_weight_t* pWeights) = 0;

	virtual void				PushDeformation(DeformationBase_t const* Deformation) = 0;
	virtual void				PopDeformation() = 0;
	virtual int					GetNumActiveDeformations() const = 0;

	virtual bool				GetMorphAccumulatorTexCoord(vec2* pTexCoord, i_morph* pMorph, int nVertex) = 0;

	// Version of get dynamic mesh that specifies a specific vertex format
	virtual IMesh* GetDynamicMeshEx(VertexFormat_t vertexFormat, bool bBuffered = true,
		IMesh* pVertexOverride = 0, IMesh* pIndexOverride = 0, material* pAutoBind = 0) = 0;

	virtual void				FogMaxDensity(float flMaxDensity) = 0;

#if defined( _X360 )
	//Seems best to expose GPR allocation to scene rendering code. 128 total to split between vertex/pixel shaders (pixel will be set to 128 - vertex). Minimum value of 16. More GPR's = more threads.
	virtual void				PushVertexShaderGPRAllocation(int iVertexShaderCount = 64) = 0;
	virtual void				PopVertexShaderGPRAllocation(void) = 0;

	virtual void				FlushHiStencil() = 0;

#endif

#if defined( _GAMECONSOLE )
	virtual void                BeginConsoleZPass(const WorldListIndicesInfo_t& indicesInfo) = 0;
	virtual void                BeginConsoleZPass2(int nSlack) = 0;
	virtual void				EndConsoleZPass() = 0;
#endif

#if defined( _PS3 )
	virtual void				FlushTextureCache() = 0;
#endif
	virtual void                AntiAliasingHint(int nHint) = 0;

	virtual material* GetCurrentMaterial() = 0;
	virtual int  GetCurrentNumBones() const = 0;
	virtual void* GetCurrentProxy() = 0;

	// Color correction related methods..
	// Client cannot call IColorCorrectionSystem directly because it is not thread-safe
	// FIXME: Make IColorCorrectionSystem threadsafe?
	virtual void EnableColorCorrection(bool bEnable) = 0;
	virtual ColorCorrectionHandle_t AddLookup(const char* pName) = 0;
	virtual bool RemoveLookup(ColorCorrectionHandle_t handle) = 0;
	virtual void LockLookup(ColorCorrectionHandle_t handle) = 0;
	virtual void LoadLookup(ColorCorrectionHandle_t handle, const char* pLookupName) = 0;
	virtual void UnlockLookup(ColorCorrectionHandle_t handle) = 0;
	virtual void SetLookupWeight(ColorCorrectionHandle_t handle, float flWeight) = 0;
	virtual void ResetLookupWeights() = 0;
	virtual void SetResetable(ColorCorrectionHandle_t handle, bool bResetable) = 0;

	//There are some cases where it's simply not reasonable to update the full screen depth texture (mostly on PC).
	//Use this to mark it as invalid and use a dummy texture for depth reads.
	virtual void SetFullScreenDepthTextureValidityFlag(bool bIsValid) = 0;

	// A special path used to tick the front buffer while loading on the 360
	virtual void SetNonInteractiveLogoTexture(texture* pTexture, float flNormalizedX, float flNormalizedY, float flNormalizedW, float flNormalizedH) = 0;
	virtual void SetNonInteractivePacifierTexture(texture* pTexture, float flNormalizedX, float flNormalizedY, float flNormalizedSize) = 0;
	virtual void SetNonInteractiveTempFullscreenBuffer(texture* pTexture, MaterialNonInteractiveMode_t mode) = 0;
	virtual void EnableNonInteractiveMode(MaterialNonInteractiveMode_t mode) = 0;
	virtual void RefreshFrontBufferNonInteractive() = 0;

	// Flip culling state (swap CCW <-> CW)
	virtual void FlipCulling(bool bFlipCulling) = 0;

	virtual void SetTextureRenderingParameter(int parm_number, texture* pTexture) = 0;

	//only actually sets a bool that can be read in from shaders, doesn't do any of the legwork
	virtual void EnableSinglePassFlashlightMode(bool bEnable) = 0;

	// Are we in Single Pass Flashlight Mode?
	virtual bool SinglePassFlashlightModeEnabled() const = 0;

	// Draws instances with different meshes
	virtual void DrawInstances(int nInstanceCount, const mesh_instance_data* pInstance) = 0;

	// Allows us to override the color/alpha write settings of a material
	virtual void OverrideAlphaWriteEnable(bool bOverrideEnable, bool bAlphaWriteEnable) = 0;
	virtual void OverrideColorWriteEnable(bool bOverrideEnable, bool bColorWriteEnable) = 0;

	virtual void ClearBuffersObeyStencilEx(bool bClearColor, bool bClearAlpha, bool bClearDepth) = 0;

	// Subdivision surface interface
	virtual int GetSubDBufferWidth() = 0;
	virtual float* LockSubDBuffer(int nNumRows) = 0;
	virtual void UnlockSubDBuffer() = 0;

	// Update current frame's game time for the shader api.
	virtual void UpdateGameTime(float flTime) = 0;

	virtual void			PrintfVA(char* fmt, va_list vargs) = 0;
	virtual void			Printf(char* fmt, ...) = 0;
	virtual float			Knob(char* knobname, float* setvalue = NULL) = 0;

	// expose scaleform functons

#if defined( INCLUDE_SCALEFORM )
	virtual void SetScaleformSlotViewport(int slot, int x, int y, int w, int h) = 0;
	virtual void RenderScaleformSlot(int slot) = 0;
	virtual void ForkRenderScaleformSlot(int slot) = 0;
	virtual void JoinRenderScaleformSlot(int slot) = 0;

	virtual void SetScaleformCursorViewport(int x, int y, int w, int h) = 0;
	virtual void RenderScaleformCursor() = 0;

	virtual void AdvanceAndRenderScaleformSlot(int slot) = 0;
	virtual void AdvanceAndRenderScaleformCursor() = 0;
#endif

	virtual void SetRenderingPaint(bool bEnable) = 0;

	// Draws batches of different materials using a faster API
	//virtual void DrawBatchedMaterials( int nBatchCount, const MaterialBatchData_t *pBatches ) = 0;

	virtual ColorCorrectionHandle_t FindLookup(const char* pName) = 0;
};
class shader
{
public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
};

class material
{
	VIRTUAL(28, color_modulate_internal(const float r, const float g, const float b),
			void(__thiscall *)(void *, float, float, float))
	(r, g, b);

public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
	VIRTUAL(1, get_group(), const char *(__thiscall *)(void *))();
	VIRTUAL(12, increment_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(13, decrement_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(27, alpha_modulate(const float a), void(__thiscall *)(void *, float))(a);
	VIRTUAL(28, color_modulate(const float r, const float g, const float b),
			void(__thiscall *)(void *, float, float, float))
	(r, g, b);
	VIRTUAL(29, set_material_var_flag(const material_var_flags flag, const bool on),
			void(__thiscall *)(void *, material_var_flags, bool))
	(flag, on);
	VIRTUAL(30, get_material_var_flag(const material_var_flags flag), bool(__thiscall *)(void *, material_var_flags))
	(flag);
	VIRTUAL(37, refresh(), void(__thiscall *)(void *))();
	VIRTUAL(47, find_var_fast(char const *name, uint32_t *token),
			material_var *(__thiscall *)(void *, const char *, uint32_t *))
	(name, token);
	VIRTUAL(68, get_shader(), shader *(__thiscall *)(void *))();

	inline void modulate(const color clr)
	{
		color_modulate_internal(clr.red() / 255.f, clr.green() / 255.f, clr.blue() / 255.f);
		alpha_modulate(clr.alpha() / 255.f);
	}
};

class model_render_t
{
public:
	VIRTUAL(0,
			draw_model(int32_t flags, client_renderable *ren, model_instance_handle inst, int32_t index, const model *m,
					   const vec3 *org, const angle *ang, int32_t skin, int32_t body, int32_t set,
					   mat3x4 *to_world = nullptr, mat3x4 *lighting = nullptr),
			int32_t(__thiscall *)(void *, int32_t, client_renderable *, model_instance_handle, int32_t, const model *,
								  const vec3 *, const angle *, int32_t, int32_t, int32_t, mat3x4 *, mat3x4 *))
	(flags, ren, inst, index, m, org, ang, skin, body, set, to_world, lighting);
	VIRTUAL(1, forced_material_override(material *mat), void(__thiscall *)(void *, material *, int, int))(mat, 0, -1);
	VIRTUAL(4, create_instance(client_renderable *ren, void *cache = nullptr),
			model_instance_handle(__thiscall *)(void *, client_renderable *, void *))
	(ren, cache);
	VIRTUAL(5, destroy_instance(model_instance_handle handle), void(__thiscall *)(void *, model_instance_handle))
	(handle);
	VIRTUAL(20,
			draw_model_setup(mat_render_context *ctx, model_render_info_t *info, draw_model_state *state, mat3x4 **out),
			void(__thiscall *)(void *, mat_render_context *, model_render_info_t *, draw_model_state *state, mat3x4 **))
	(ctx, info, state, out);
	VIRTUAL(24, suppress_engine_lighting(bool suppress), void(__thiscall *)(void *, bool))(suppress);
};

enum render_target_size_mode
{
	rt_size_no_change,
	rt_size_default,
	rt_size_picmip,
	rt_size_hdr,
	rt_size_full_frame_buffer,
	rt_size_offscreen,
	rt_size_full_frame_buffer_rounded_up
};

enum material_render_target_depth
{
	mat_rt_depth_shared,
	mat_rt_depth_separate,
	mat_rt_depth_none,
	mat_rt_depth_only
};

enum compiled_vtf_flags
{
	textureflags_pointsample = 0x00000001,
	textureflags_trilinear = 0x00000002,
	textureflags_clamps = 0x00000004,
	textureflags_clampt = 0x00000008,
	textureflags_anisotropic = 0x00000010,
	textureflags_hint_dxt5 = 0x00000020,
	textureflags_pwl_corrected = 0x00000040,
	textureflags_normal = 0x00000080,
	textureflags_nomip = 0x00000100,
	textureflags_nolod = 0x00000200,
	textureflags_all_mips = 0x00000400,
	textureflags_procedural = 0x00000800,
	textureflags_onebitalpha = 0x00001000,
	textureflags_eightbitalpha = 0x00002000,
	textureflags_envmap = 0x00004000,
	textureflags_rendertarget = 0x00008000,
	textureflags_depthrendertarget = 0x00010000,
	textureflags_nodebugoverride = 0x00020000,
	textureflags_singlecopy = 0x00040000,
	textureflags_srgb = 0x00080000,
	textureflags_default_pool = 0x00100000,
	textureflags_combined = 0x00200000,
	textureflags_async_download = 0x00400000,
	textureflags_nodepthbuffer = 0x00800000,
	textureflags_skip_initial_download = 0x01000000,
	textureflags_clampu = 0x02000000,
	textureflags_vertextexture = 0x04000000,
	textureflags_ssbump = 0x08000000,
	textureflags_most_mips = 0x10000000,
	textureflags_border = 0x20000000,
};

class material_system_t
{
public:
	VIRTUAL(36, get_back_buffer_format(), int(__thiscall *)(void *))();
	VIRTUAL(83, create_material(const char *name, keyvalues *kv),
			material *(__thiscall *)(void *, const char *, keyvalues *))
	(name, kv);
	VIRTUAL(84, find_material(const char *mat, const char *group = nullptr),
			material *(__thiscall *)(void *, const char *, const char *, bool, const char *))
	(mat, group, true, nullptr);
	VIRTUAL(86, first_material(), uint16_t(__thiscall *)(void *))();
	VIRTUAL(87, next_material(uint16_t handle), uint16_t(__thiscall *)(void *, uint16_t))(handle);
	VIRTUAL(88, invalid_material(), uint16_t(__thiscall *)(void *))();
	VIRTUAL(89, get_material(uint16_t handle), material *(__thiscall *)(void *, uint16_t))(handle);
	VIRTUAL(91, find_texture(const char *name, const char *groupname, bool complain = true, int32_t flags = 0),
			texture *(__thiscall *)(void *, const char *, const char *, bool, int32_t))
	(name, groupname, complain, flags);
	VIRTUAL(94, begin_render_target_allocation(), void(__thiscall *)(void *))();
	VIRTUAL(95, end_render_target_allocation(), void(__thiscall *)(void *))();
	VIRTUAL(97,
			create_named_render_target_texture_ex(const char *name, int w, int h, int size_mode, int format, int depth,
												  uint32_t flags, uint32_t target_flags),
			texture *(__thiscall *)(void *, const char *, int, int, int, int, int, uint32_t, uint32_t))
	(name, w, h, size_mode, format, depth, flags, target_flags);
	VIRTUAL(115, get_render_context(), mat_render_context *(__thiscall *)(void *))();
	VIRTUAL(136, finish_render_target_allocation(), void(__thiscall *)(void *))();

	VAR(globals::material_system, disable_render_target_allocation_forever, bool);
};

class studio_render_t
{
public:
	VIRTUAL(20, set_ambient_light_colors(vec4 *cols), void(__thiscall *)(void *, vec4 *))(cols);
	VIRTUAL(22, set_local_lights(int num, void *desc), void(__thiscall *)(void *, int, void *))(num, desc);
};

struct mdl_sequence_layer_t
{
	int sequence;
	float weight;
};

inline constexpr auto max_studio_flex_controls = 96 * 4;

struct mdl_t
{
	char pad0[48];
	material *override;
	texture *env_cubemap;
	texture *hdr_env_cubemap;
	uint16_t handle;
	char pad1[8];
	color col;
	char pad2[2];
	int skin;
	int body;
	int sequence;
	int lod;
	float playback_rate;
	float time;
	float current_anim_end_time;
	float flex_controls[max_studio_flex_controls];
	vec3 view_target;
	bool world_space_view_target;
	bool use_sequence_playback_fps;
	char pad3[2];
	void *proxy_data;
	float time_basis_adjustment;
	char pad4[0x1C];
};

struct mdl_data_t
{
	mdl_t mdl;
	mat3x4 mdl_to_world;
	bool request_bone_merge_takeover;
};

class render_to_rt_helper_object_t
{
public:
	virtual void draw(const mat3x4 &root_to_world) = 0;
	virtual bool bounding_sphere(vec3 &center, float &radius) = 0;
	virtual texture *get_env_cube_map() = 0;
};

class merged_mdl_t : public render_to_rt_helper_object_t
{
public:
	virtual ~merged_mdl_t() {}
	virtual void set_mdl(uint16_t handle, void *owner, void *data) = 0;
	virtual void set_mdl(const char *name, void *owner, void *data) = 0;

	FUNC(game->client.at(functions::merged_mdl::init), init(), void(__thiscall *)(void *))();
	FUNC(game->client.at(functions::merged_mdl::set_merge_mdl), set_merge_mdl(const char *name),
		 uint16_t(__thiscall *)(void *, const char *, void *, void *, bool))
	(name, nullptr, nullptr, false);
	FUNC(game->client.at(functions::merged_mdl::setup_bones_for_attachment_queries),
		 setup_bones_for_attachment_queries(), void(__thiscall *)(void *))
	();

	void set_sequence(const int nSequence, const bool bUseSequencePlaybackFPS)
	{
		this->root.mdl.sequence = nSequence;
		this->root.mdl.use_sequence_playback_fps = bUseSequencePlaybackFPS;
		this->root.mdl.time_basis_adjustment = this->root.mdl.time;
	}


	mdl_data_t root;
	mdl_data_t *merge;
	int merge_alloc;
	int merge_grow;
	int merge_size;
	mdl_data_t *merge_els;
	float pose_parameters[24];
	void *layers;
	int layers_alloc;
	int layers_grow;
	int layers_size;
	void *layers_els;
};
} // namespace sdk

#endif // SDK_MODEL_RENDER_H
