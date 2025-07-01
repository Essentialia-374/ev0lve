
#ifndef SDK_RENDER_VIEW_H
#define SDK_RENDER_VIEW_H

#include <sdk/color.h>
#include <sdk/macros.h>
#include <sdk/math.h>

namespace sdk
{
struct v_plane
{
	vec3 normal;
	float dist;
};

using frustum_t = v_plane[6];

enum clear_flags
{
	view_clear_color = 1,
	view_clear_depth = 2,
	view_clear_full_target = 4,
	view_no_draw = 8,
	view_clear_obey_stencil = 16,
	view_clear_stencil = 32
};

class render_view_t
{
public:
	virtual void      DrawBrushModel(void* pBaseEntity, void* pModel, const vec3& vecOrigin, const vec3& angView, bool bSort) = 0;
	virtual void      DrawIdentityBrushModel(void* pList, void* pModel) = 0;
	virtual void      TouchLight(struct dlight_t* dLight) = 0;
	virtual void      Draw3DDebugOverlays() = 0;
	virtual void      SetBlend(float flBlend) = 0;
	virtual float      GetBlend() = 0;
	virtual void      SetColorModulation(float const* flBlend) = 0;
	virtual void      GetColorModulation(float* flBlend) = 0;
	virtual void      SceneBegin() = 0;
	virtual void      SceneEnd() = 0;
	virtual void      GetVisibleFogVolume(const vec3& vecEyePoint, void* pInfo) = 0;
	virtual void* CreateWorldList() = 0;
	virtual void      BuildWorldLists_Epilogue(void* pList, void* pInfo, bool bShadowDepth) = 0;
	virtual void      BuildWorldLists(void* pList, void* pInfo, int iForceFViewLeaf, const void* pVisData = nullptr, bool bShadowDepth = false, float* pReflectionWaterHeight = nullptr) = 0;
	virtual void      DrawWorldLists(void* pList, unsigned long flags, float flWaterZAdjust) = 0;
	virtual void      GetWorldListIndicesInfo(void* pIndicesInfoOut, void* pList, unsigned long nFlags) = 0;
	virtual void      DrawTopView(bool bEnable) = 0;
	virtual void      TopViewNoBackfaceCulling(bool bDisable) = 0;
	virtual void      TopViewNoVisCheck(bool bDisable) = 0;
	virtual void      Unused0() = 0;// TopViewBounds(vec32D const& vecMins, vec32D const& vecMaxs) = 0;
	virtual void      SetTopViewVolumeCuller(const void* pVolumeCuller) = 0;
	virtual void      DrawLights() = 0;
	virtual void      DrawMaskEntities() = 0;
	virtual void      DrawTranslucentSurfaces(void* pList, int* pSortList, int iSortCount, unsigned long fFlags) = 0;
	virtual void      DrawLineFile() = 0;
	virtual void      DrawLightmaps(void* pList, int iPageID) = 0;
	virtual void      ViewSetupVis(bool bNoVis, int nOrigins, const vec3 vecOrigin[]) = 0;
	virtual bool      AreAnyLeavesVisible(int* pLeafList, int nLeaves) = 0;
	virtual  void      VguiPaint() = 0;
	virtual void      ViewDrawFade(uint8_t* pColor, material* pMaterial) = 0;
	virtual void      OLD_SetProjectionMatrix(float flFov, float zNear, float zFar) = 0;
	virtual void      Unused1() = 0;// ColorVec_t    GetLightAtPoint(vec3& vecPosition) = 0;
	virtual int        GetViewEntity() = 0;
	virtual bool      IsViewEntity(int nEntityIndex) = 0;
	virtual float      GetFieldOfView() = 0;
	virtual unsigned char** GetAreaBits() = 0;
	virtual void      SetFogVolumeState(int nVisibleFogVolume, bool bUseHeightFog) = 0;
	virtual void      InstallBrushSurfaceRenderer(void* pBrushRenderer) = 0;
	virtual void      DrawBrushModelShadow(void* pRenderable) = 0;
	virtual  bool      LeafContainsTranslucentSurfaces(void* pList, int nSortIndex, unsigned long fFlags) = 0;
	virtual bool      DoesBoxIntersectWaterVolume(const vec3& vecMins, const vec3& vecMaxs, int nLeafWaterDataID) = 0;
	virtual void      SetAreaState(unsigned char uAreaBits[32], unsigned char uAreaPortalBits[24]) = 0;
	virtual void      VGui_Paint(int nMode) = 0;
	virtual void      Push3DView(void* pRenderContext, const view_setup& view, int nFlags, texture* pRenderTarget, frustum_t frustumPlanes) = 0;
	virtual void      Push2DView(void* pRenderContext, const view_setup& view, int nFlags, texture* pRenderTarget, frustum_t frustumPlanes) = 0;
	virtual void      PopView(void* pRenderContext, frustum_t frustumPlanes) = 0;
	virtual void      SetMainView(const vec3& vecOrigin, const vec3& angView) = 0;
	virtual void      ViewSetupVisEx(bool bNoVis, int nOrigins, const vec3 arrOrigin[], unsigned int& uReturnFlags) = 0;
	virtual void      OverrideViewFrustum(frustum_t custom) = 0;
	virtual void      DrawBrushModelShadowDepth(void* pEntity, void* pModel, const vec3& vecOrigin, const vec3& angView, int nDepthMode) = 0;
	virtual void      UpdateBrushModelLightmap(void* pModel, void* pRenderable) = 0;
	virtual void      BeginUpdateLightmaps() = 0;
	virtual void      EndUpdateLightmaps() = 0;
	virtual void      OLD_SetOffCenterProjectionMatrix(float flFOV, float flNearZ, float flFarZ, float flAspectRatio, float flBottom, float flTop, float flLeft, float flRight) = 0;
	virtual void      OLD_SetProjectionMatrixOrtho(float flLeft, float flTop, float flRight, float flBottom, float flNearZ, float flFarZ) = 0;
	virtual void      Push3DView(void* pRenderContext, const view_setup& view, int nFlags, texture* pRenderTarget, frustum_t frustumPlanes, texture* pDepthTexture) = 0;
	virtual void      GetMatricesForView(const view_setup& view, vmatrix* pWorldToView, vmatrix* pViewToProjection, vmatrix* pWorldToProjection, vmatrix* pWorldToPixels) = 0;
	virtual void      DrawBrushModelEx(void* pEntity, void* pModel, const vec3& vecOrigin, const vec3& angView, int nMode) = 0;
	virtual bool      DoesBrushModelNeedPowerOf2Framebuffer(const void* pModel) = 0;
	virtual void      DrawBrushModelArray(void* pContext, int nCount, const void* pInstanceData, int nModelTypeFlags) = 0;
};
} // namespace sdk

#endif // SDK_RENDER_VIEW_H
