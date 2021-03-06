#version 400 compatibility
#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_texture_array : enable
#pragma import_defines (OSG_LIGHTING, OSG_FOG_MODE)

uniform bool osg_ReceiveShadow;

float ov_getShadow(vec3 normal, float depth);

vec3 ov_directionalLightAndShadow(vec3 normal, vec3 diffuse, float depth)
{
#if defined(OSG_LIGHTING)
	vec3 light_dir = normalize(gl_LightSource[0].position.xyz);
	float NdotL = max(dot(normal, light_dir), 0.0);
	if(osg_ReceiveShadow)
		NdotL *= ov_getShadow(normal, depth);
	vec3 light = min(NdotL * diffuse*gl_LightSource[0].diffuse.xyz + gl_LightSource[0].ambient.xyz, 1.0);
    
	//float NdotHV = max(dot(normal, gl_LightSource[0].halfVector.xyz), 0.0);
	//if ( NdotL * NdotHV > 0.0 )
	//	light += gl_LightSource[0].specular.xyz * pow( NdotHV, 1.0);
    //light += gl_FrontLightProduct[0].specular.xyz * pow( NdotHV, gl_FrontMaterial.shininess );
	return light;
#else
	return diffuse;
#endif
}

vec3 ov_applyFog(vec3 color, float depth)
{
#ifdef OSG_FOG_MODE
	#if OSG_FOG_MODE == 1 //LINEAR
		float fog_factor = (gl_Fog.end - depth) * gl_Fog.scale;
	#elif OSG_FOG_MODE == 2 //EXP
		float fog_factor = exp(-gl_Fog.density * depth);
	#elif OSG_FOG_MODE == 3 //EXP2
		float fog_factor = exp(-pow((gl_Fog.density * depth), 2.0));
	#endif
	fog_factor = clamp(fog_factor, 0.0, 1.0);
	color.xyz = mix(gl_Fog.color.xyz, color.xyz, fog_factor);
#endif
	return color;
}

vec3 ov_lit(vec3 color, vec3 diffuse,vec3 normal, float depth)
{
	color *= ov_directionalLightAndShadow(normal,diffuse,depth);
	color = ov_applyFog(color, depth);
	return color;
}