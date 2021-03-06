#version 400 compatibility //we use gl_Fog
#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_texture_array : enable

uniform mat4 osg_ModelViewMatrix;

in ov_VertexData
{
  vec4 Position;
  vec3 Normal;
  vec2 TexCoord0;
} ov_in;

out vec4 fragColor;

vec3 ov_lit(vec3 color, vec3 diffuse,vec3 normal, float depth);
vec4 ov_getTerrainColor(float depth,vec2 tex_coord0, vec2 terrain_pos);

void main(void)
{
	vec4 mvpos = osg_ModelViewMatrix * ov_in.Position;
	float depth = length(mvpos);
	//vec4 terrain_color = ov_getTerrainColor(depth);
	vec4 terrain_color = ov_getTerrainColor(depth, ov_in.TexCoord0, ov_in.Position.xy);
	//apply lighting and fog
	terrain_color.xyz = ov_lit(terrain_color.xyz, vec3(1.0), normalize(ov_in.Normal),depth);
	fragColor = terrain_color;
}