#version 400 compatibility
#pragma import_defines (OV_TERRAIN_TESSELLATION)

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;

#define USE_COMP_PROFILE
#ifndef USE_COMP_PROFILE //Core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_MultiTexCoord0;
#endif

out ov_VertexData
{
  vec4 Position;
  vec3 Normal;
  vec2 TexCoord0;
} ov_out;

#ifdef OV_TERRAIN_TESSELLATION
#else
void ov_setShadowTexCoords(vec4 mv_pos);
#endif

vec4 ov_applyTerrainElevation(vec4 pos, vec2 tex_coords);

void main()
{
#ifdef USE_COMP_PROFILE
	vec4 osg_Vertex = gl_Vertex;
	vec3 osg_Normal= gl_Normal;
	vec4 osg_MultiTexCoord0 = gl_MultiTexCoord0;
#endif
	ov_out.TexCoord0 = osg_MultiTexCoord0.xy;
	ov_out.Position = ov_applyTerrainElevation(osg_Vertex, osg_MultiTexCoord0.xy);
#ifdef OV_TERRAIN_TESSELLATION
	ov_out.Normal = osg_Normal;
#else
	ov_out.Normal = normalize(osg_NormalMatrix * osg_Normal);
	gl_Position = osg_ModelViewProjectionMatrix * ov_out.Position;
	
	//TODO: fix compability
	vec4 mv_pos = osg_ModelViewMatrix * ov_out.Position;
	ov_setShadowTexCoords(mv_pos);
#endif
}
