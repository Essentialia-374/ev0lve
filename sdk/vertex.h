
#ifndef SDK_VERTEX_H
#define SDK_VERTEX_H

#include <sdk/vec2.h>

namespace sdk
{
	typedef uint64_t VertexFormat_t;
	enum e_image_format
	{
		IMAGE_FORMAT_UNKNOWN = -1,
		IMAGE_FORMAT_RGBA8888 = 0,
		IMAGE_FORMAT_ABGR8888,
		IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_BGR888,
		IMAGE_FORMAT_RGB565,
		IMAGE_FORMAT_I8,
		IMAGE_FORMAT_IA88,
		IMAGE_FORMAT_P8,
		IMAGE_FORMAT_A8,
		IMAGE_FORMAT_RGB888_BLUESCREEN,
		IMAGE_FORMAT_BGR888_BLUESCREEN,
		IMAGE_FORMAT_ARGB8888,
		IMAGE_FORMAT_BGRA8888,
		IMAGE_FORMAT_DXT1,
		IMAGE_FORMAT_DXT3,
		IMAGE_FORMAT_DXT5,
		IMAGE_FORMAT_BGRX8888,
		IMAGE_FORMAT_BGR565,
		IMAGE_FORMAT_BGRX5551,
		IMAGE_FORMAT_BGRA4444,
		IMAGE_FORMAT_DXT1_ONEBITALPHA,
		IMAGE_FORMAT_BGRA5551,
		IMAGE_FORMAT_UV88,
		IMAGE_FORMAT_UVWQ8888,
		IMAGE_FORMAT_RGBA16161616F,
		IMAGE_FORMAT_RGBA16161616,
		IMAGE_FORMAT_UVLX8888,
		IMAGE_FORMAT_R32F,
		IMAGE_FORMAT_RGB323232F,
		IMAGE_FORMAT_RGBA32323232F,
		IMAGE_FORMAT_NV_DST16,
		IMAGE_FORMAT_NV_DST24,
		IMAGE_FORMAT_NV_INTZ,
		IMAGE_FORMAT_NV_RAWZ,
		IMAGE_FORMAT_ATI_DST16,
		IMAGE_FORMAT_ATI_DST24,
		IMAGE_FORMAT_NV_NULL,
		IMAGE_FORMAT_ATI2N,
		IMAGE_FORMAT_ATI1N,
		IMAGE_FORMAT_DXT1_RUNTIME,
		IMAGE_FORMAT_DXT5_RUNTIME,
		NUM_IMAGE_FORMATS
	};

struct vertex_t
{
	vec2 pos;
	vec2 cord;

	vertex_t() = default;
	vertex_t(const vec2 &pos, const vec2 &coord = vec2(0, 0)) : pos(pos), cord(coord) {}
};
enum VertexFormatFlags_t
{
	// Indicates an uninitialized VertexFormat_t value
	VERTEX_FORMAT_INVALID = 0xFFFFFFFF,

	VERTEX_POSITION = 0x0001,
	VERTEX_NORMAL = 0x0002,
	VERTEX_cl = 0x0004,
	VERTEX_SPECULAR = 0x0008,

	VERTEX_TANGENT_S = 0x0010,
	VERTEX_TANGENT_T = 0x0020,
	VERTEX_TANGENT_SPACE = VERTEX_TANGENT_S | VERTEX_TANGENT_T,

	// Indicates we're using wrinkle
	VERTEX_WRINKLE = 0x0040,

	// Indicates we're using bone indices
	VERTEX_BONE_INDEX = 0x0080,

	// Indicates this expects a color stream on stream 1
	VERTEX_cl_STREAM_1 = 0x0100,

	// Indicates this format shouldn't be bloated to cache align it
	// (only used for VertexUsage)
	VERTEX_FORMAT_USE_EXACT_FORMAT = 0x0200,

	// Indicates that compressed vertex elements are to be used (see also VertexCompressionType_t)
	VERTEX_FORMAT_COMPRESSED = 0x400,

	// Position or normal (if present) should be 4D not 3D
	VERTEX_FORMAT_PAD_POS_NORM = 0x800,

	// Update this if you add or remove bits...
	VERTEX_LAST_BIT = 11,

	VERTEX_BONE_WEIGHT_BIT = VERTEX_LAST_BIT + 1,
	USER_DATA_SIZE_BIT = VERTEX_LAST_BIT + 4,
	TEX_COORD_SIZE_BIT = VERTEX_LAST_BIT + 7,

	VERTEX_BONE_WEIGHT_MASK = (0x7 << VERTEX_BONE_WEIGHT_BIT),
	USER_DATA_SIZE_MASK = (0x7 << USER_DATA_SIZE_BIT),

	VERTEX_FORMAT_FIELD_MASK = 0x0FF,

	// If everything is off, it's an unknown vertex format
	VERTEX_FORMAT_UNKNOWN = 0,
};

struct VertexStreamSpec_t
{
	enum StreamSpec_t
	{
		STREAM_DEFAULT,		// stream 0: with position, normal, etc.
		STREAM_SPECULAR1,	// stream 1: following specular vhv lighting
		STREAM_FLEXDELTA,	// stream 2: flex deltas
		STREAM_MORPH,		// stream 3: morph
		STREAM_UNIQUE_A,	// unique stream 4
		STREAM_UNIQUE_B,	// unique stream 5
		STREAM_UNIQUE_C,	// unique stream 6
		STREAM_UNIQUE_D,	// unique stream 7
		STREAM_SUBDQUADS,	// stream 8: quad buffer for subd's
	};

	enum
	{
		MAX_UNIQUE_STREAMS = 4
	};

	VertexFormatFlags_t iVertexDataElement;
	StreamSpec_t iStreamSpec;
};
enum MaterialPrimitiveType_t
{
	MATERIAL_POINTS = 0x0,
	MATERIAL_LINES,
	MATERIAL_TRIANGLES,
	MATERIAL_TRIANGLE_STRIP,
	MATERIAL_LINE_STRIP,
	MATERIAL_LINE_LOOP,	// a single line loop
	MATERIAL_POLYGON,	// this is a *single* polygon
	MATERIAL_QUADS,
	MATERIAL_SUBD_QUADS_EXTRA, // Extraordinary sub-d quads
	MATERIAL_SUBD_QUADS_REG,   // Regular sub-d quads
	MATERIAL_INSTANCED_QUADS, // (X360) like MATERIAL_QUADS, but uses vertex instancing

	// This is used for static meshes that contain multiple types of
	// primitive types.	When calling draw, you'll need to specify
	// a primitive type.
	MATERIAL_HETEROGENOUS
};

inline VertexStreamSpec_t* FindVertexStreamSpec(VertexFormat_t iElement, VertexStreamSpec_t* arrVertexStreamSpec)
{
	for (; arrVertexStreamSpec &&
		arrVertexStreamSpec->iVertexDataElement != VERTEX_FORMAT_UNKNOWN;
		++arrVertexStreamSpec)
	{
		if (arrVertexStreamSpec->iVertexDataElement == iElement)
			return arrVertexStreamSpec;
	}
	return NULL;
}
} // namespace sdk



#endif // SDK_VERTEX_H
