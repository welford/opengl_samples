-- Shared

#version 420 core
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec3 inNormal;

layout(std140, binding=1) uniform Transforms
{
uniform mat4		mvp;
uniform mat4		proj;
uniform mat4		mv;
uniform mat4		nrmn;
}trans;


-- MRT.Stage1.Vertex

#version 420 core
layout(location = 0) in vec3 inVertex;
out vec3 colour;
void main()
{
	colour = vec3(1,1,1);
	gl_Position = vec4(inVertex,1);
	gl_PointSize = 10;
}

-- MRT.Stage1.Fragment

#version 420 core
in vec3 colour;
layout(location = 0) out vec4 outFragColour[3]; //fils up locations 0, 1, 2
void main()
{
	outFragColour[0] = vec4(colour.r,0,0,1);
	outFragColour[1] = vec4(0,colour.g,0,1);
	outFragColour[2] = vec4(0,0,colour.b,1);
}

-- MRT_2.Stage1.Fragment

#version 420 core
in vec3 colour;
layout(location = 0) out vec4 outFragColour[4];  //fils up locations 0, 1, 2, 3
void main()
{
	outFragColour[0] = vec4(colour.r,0,0,1);
	outFragColour[1] = vec4(0,colour.g,0,1);
	outFragColour[2] = vec4(0,0,colour.b,1);
	outFragColour[3] = vec4(colour.rgb,1);
}

-- MRT.Stage2.Vertex

#version 420 core
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
uniform float scale = 0.9;
uniform vec3 translate = vec3(0,0,0);

out vec2 tex;
void main()
{
	tex = inTexCoord;
	gl_PointSize = 10;
	gl_Position = vec4((inVertex.xyz*scale)+translate,1);
}

-- MRT.Stage2.Fragment

#version 420 core
in vec2 tex;
out vec4 fragColour;
uniform sampler2D mrt;
void main()
{
	fragColour = texture(mrt, tex.st);
}

-- BasicVertexShader

out vec3 colour;

void main()
{
	colour = inColour;
	vec4 finalPosition = trans.mvp * vec4(inVertex,1) ;
	gl_Position = finalPosition;
}

-- BasicFragmentShader

#version 420 core
in vec3 colour;
out vec4 fragColour;

void main()
{
	fragColour = vec4(colour.rgb,1);
}

-- Dbg.ScreenSpaceTexture.Vertex

#version 420 core
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
uniform float scale = 1.0;
uniform vec3 translate = vec3(0,0,0);

out vec2 tex;
void main()
{
	tex = inTexCoord;
	gl_Position = vec4((inVertex.xyz*scale)+translate,1);
}

-- Dbg.ScreenSpaceTexture.Fragment

#version 420 core
in vec2 tex;
out vec4 fragColour;
layout(binding=0) uniform sampler2D tex_smp;
void main()
{
	fragColour = texture(tex_smp, tex.st);
}

-- SingleShadow.Vertex

out vec3 colour;
out vec4 shadowPosition;
uniform mat4 shadowMtx;

void main()
{
	colour = inColour;
	shadowPosition = shadowMtx * vec4(inVertex,1);
	//shadowPosition.w = 1.0/shadowPosition.w;	
	gl_Position = trans.mvp * vec4(inVertex,1);
}

-- SingleShadow.Fragment

#version 420 core
in vec3 colour;
in vec4 shadowPosition;
out vec4 fragColour;
uniform sampler2D shadowMap;

void main()
{
	float shadeFactor = 1.0;	
	vec4 shadowCoord = shadowPosition / shadowPosition.w;		
	shadowCoord.z -= 0.005;
	if(!(shadowCoord.s >= 1.0 || shadowCoord.s <= 0.0 || shadowCoord.t >= 1.0 || shadowCoord.t <= 0.0))
	{
		float distanceFromLight = 0.0;
		distanceFromLight = texture(shadowMap,shadowCoord.xy).z;
		if(distanceFromLight < shadowCoord.z) 
			shadeFactor = 0.5;
	}
	fragColour = vec4(colour.rgb*shadeFactor,1);
}

-- MultiShadow.Vertex
//#define NUM_SHADOWS 10 //set externally
out vec3 colour;
out vec4 shadowPosition[NUM_SHADOWS];

layout(std140, binding=2) uniform Shadows
{
	//uniform mat4 dummy;		//an Nvidia driver bug meant we needed this, but no longer!
	uniform mat4 shadowMtx[NUM_SHADOWS];
}sdw;

void main()
{
	colour = inColour;
	for(unsigned int i=0;i<NUM_SHADOWS;i++)
		shadowPosition[i] = sdw.shadowMtx[i] * vec4(inVertex,1);
	gl_Position = trans.mvp * vec4(inVertex,1);
}

-- MultiShadow.Fragment
//#define NUM_SHADOWS 10 //set externally
#version 420 core
in vec3 colour;
in vec4 shadowPosition[NUM_SHADOWS];
out vec4 fragColour;
layout(binding=0) uniform sampler2D shadowMap[NUM_SHADOWS];

void main()
{
	float shadeFactor = 1.0;
	vec4 shadowCoord;
	float distanceFromLight;
	for(unsigned int i=0;i<NUM_SHADOWS;i++)
	{
		shadowCoord = shadowPosition[i] / shadowPosition[i].w;		
		shadowCoord.z -= 0.005;
		if(!(shadowCoord.s >= 1.0 || shadowCoord.s <= 0.0 || shadowCoord.t >= 1.0 || shadowCoord.t <= 0.0))
		{
			distanceFromLight = texture(shadowMap[i],shadowCoord.xy).z;
			if(distanceFromLight < shadowCoord.z) 
				shadeFactor *= 0.9;
		}		
	}
	fragColour = vec4(colour.rgb*shadeFactor,1);
}

-- PassThroughVertex
#version 420 core
layout(location = 0) in vec3 inVertex;
void main()
{
	gl_Position = vec4(inVertex,1) ;
}

-- PassThroughGeometry
#version 420 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

void main()
{
	for(int i = 0; i < gl_in.length(); i++)
	{
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
}

-- PassThroughFragment
#version 420 core
//in vec3 colour;
out vec4 fragColour;
void main()
{
	fragColour = vec4(1,1,1,1);
}

-- AddColorGeometry
#version 420 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
out vec3 color;

void main()
{
	for(int i = 0; i < gl_in.length(); i++)
	{
		gl_Position = gl_in[i].gl_Position;
		if(i==0)
			color = vec3(1.0,0,0);
		else if(i==1)
			color = vec3(0,1.0,0);
		else
			color = vec3(0,0,1.0);

		EmitVertex();
	}
}

-- AddColorFragment
#version 420 core
in vec3 color;
out vec4 fragColour;
void main()
{
	fragColour = vec4(color,1);
}

-- DuplicatePrimitiveGeometry
#version 420 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 6) out;

void main()
{
	for(int i = 0; i < gl_in.length; i++)
	{
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();

	for(int i = 0; i < gl_in.length; i++)
	{
		gl_Position = gl_in[i].gl_Position+vec4(0.5,0,0,0);
		EmitVertex();
	}
	EndPrimitive();
}



-- FlatShading.Vertex

out vec3 colour;
out vec4 position;
out vec4 light;

uniform vec3 clr = vec3(0.5, 0.5, 0.5);
uniform vec4 lgt = vec4(-1,-1,0,0);

void main()
{
	colour = clr;
	light = lgt;
	position = trans.mvp * vec4(inVertex,1) ;
	gl_Position = position;
}


-- FlatShading.VertexClr

out vec3 colour;
out vec4 position;
out vec4 light;

uniform vec4 lgt = vec4( -1, -1, 0.3, 0 );

void main()
{
	colour = inColour;
	light = lgt;
	position = trans.mvp * vec4(inVertex,1) ;
	gl_Position = position;
}

-- FlatShading.Fragment

#version 420 core
in vec3 colour;
in vec4 light;
in vec4 position;

out vec4 fragColour;

void main()
{
	vec3 normal = normalize(cross(dFdx(position.xyz), dFdy(position.xyz)));
	float ndl = max(dot(normal, normalize(light.xyz)),0.0);
	fragColour = vec4(colour.rgb*ndl,1);
}

-- CubemapReflections.Vertex

out vec3 colour;
out vec3 eye_dir;
out vec3 normal;
//out vec3 cubemap_lookup;

uniform mat4 inverse_view = mat4(1.0f);
uniform mat4 model_matrix = mat4(1.0f);

void main()
{
	//vec3 eye_dir, normal;
	vec4 pos;

	colour = inColour;	

	pos = trans.mv * vec4(inVertex, 1);
	eye_dir = pos.xyz/pos.w;
	normal = (trans.nrmn * vec4(inNormal, 0)).xyz;		
	//cubemap_lookup = reflect( eye_dir,  normal );
	//cubemap_lookup = ( inverse_view * vec4(cubemap_lookup, 0) ).xyz;	

	//eye_dir = (model_matrix * vec4(inVertex, 1)).xyz;
	//normal = (model_matrix * vec4(inNormal, 0)).xyz;		
	//cubemap_lookup = reflect( eye_dir,  normal );	
	

	gl_Position = trans.mvp * vec4(inVertex,1);
}

-- CubemapReflections.Fragment
uniform samplerCube cube_map;
uniform mat4 inverse_view = mat4(1.0f);

in vec3 eye_dir;
in vec3 normal;

//in vec3 cubemap_lookup;

out vec4 fragColour;

void main()
{
	vec3 cubemap_lookup = reflect( eye_dir,  normal );
	cubemap_lookup = ( inverse_view * vec4(cubemap_lookup, 0) ).xyz;	

	vec4 cube_map_colour = texture( cube_map, cubemap_lookup );
	fragColour = vec4(cube_map_colour.rgb, 1);	
}

-- LocalCubemapReflections.Vertex

out vec3 colour;
out vec3 eye_dir;
out vec3 normal;

uniform mat4 inverse_view = mat4(1.0f);
uniform mat4 model_matrix = mat4(1.0f);

void main()
{
	vec4 pos;
	colour = inColour;	
	pos = trans.mv * vec4(inVertex, 1);
	eye_dir = normalize(pos.xyz/pos.w);
	normal = normalize((trans.nrmn * vec4(inNormal, 0)).xyz);
	gl_Position = trans.mvp * vec4(inVertex,1);
}

-- LocalCubemapReflections.Fragment

uniform samplerCube cube_map;
uniform mat4 inverse_view = mat4(1.0f);

in vec3 eye_dir;
in vec3 normal;

out vec4 fragColour;

void main()
{
	vec3 cubemap_lookup = reflect( eye_dir,  normal );
	vec3 ws_cubemap_lookup = ( inverse_view * vec4(cubemap_lookup, 0) ).xyz;	//bascially in Cube Space

	vec3 cs_pos = vec3(9.0, 0, 0);

	//ray segment vs sphere
	//	where 
	//	- m is start of ray(P) minus the center of the circle (C), P-C 
	//	- d is the normalized direction of the ray
	//	- t is the length along the ray 
	//	- r is the radius of the circle
	// t^2 + 2(m.d)t + (m.m) - r^2 = 0
	// which is quadratic equation with the terms
	// a = t^2
	// b = 2(m.d) 	
	// c = m.m - r^2

	float b = 2.0 * dot( ws_cubemap_lookup, cs_pos );
	float c = dot( cs_pos, cs_pos ) - 1.0*1.0;
	float discrim = b * b - 4.0 * c;
	bool hasIntersects = false;
	
	//vec4 reflColor = vec4(1, 0, 0, 0);
	float nearT = 0;
	vec4 cube_map_colour = vec4(1, 0, 0, 0);

	if (discrim > 0) {
		// pick a small error value very close to zero as "epsilon"
		discrim = sqrt(discrim);
		nearT = -(discrim-b)/2.0f;
		//hasIntersects = true;//((abs(sqrt(discrim) - b) / 2.0) > 0.00001);
		if(nearT <= 0.0001){
			nearT = (discrim - b)/2.0f;
			hasIntersects = (nearT > 0.0001) ? true : false;
		}
	}
	if (hasIntersects) {
		// determine where on the unit sphere reflVect intersects
		ws_cubemap_lookup = nearT * ws_cubemap_lookup - cs_pos;
		// reflVect.y = -reflVect.y; // optional - see text
		// now use the new intersection location as the 3D direction
		cube_map_colour = texture( cube_map, ws_cubemap_lookup);
	}
	//vec4 cube_map_colour = texture( cube_map, ws_cubemap_lookup);
	fragColour = vec4(cube_map_colour.rgb, 1);	
}

-- Skymap.Vertex

out vec3 cubemap_lookup;

void main()
{
	cubemap_lookup = inVertex;
	gl_Position = trans.mvp * vec4(inVertex,1);
}

-- Skymap.Fragment

uniform samplerCube cube_map;
in vec3 cubemap_lookup;
out vec4 fragColour;

void main()
{
	fragColour = texture( cube_map, cubemap_lookup );
}

-- Gamma.Vertex

#version 420 core
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
uniform float scale = 1.0;
uniform vec3 translate = vec3(0,0,0);
out vec2 tex;

void main()
{
	tex = inTexCoord;
	gl_Position = vec4((inVertex.xyz*scale)+translate,1);
}

-- Gamma.Fragment

#version 420 core
in vec2 tex;
out vec4 fragColour;
uniform sampler2D src_image;

uniform float contrast	= 1.0;
uniform float brightness = 0.0;
uniform float inverse_gamma = 1.0/1.0;

void main()
{
	//vec4 clr = texture(src_image, tex.st);
	//clr.rgb = clr.rgb + brightness;
	//clr.rgb = ((clr.rgb-vec3(0.5)) * contrast) + vec3(0.5);
	//fragColour = vec4(pow(clr.rgb,vec3(inverse_gamma)),clr.a);

	vec4 clr = texture(src_image, tex.st);
	fragColour = vec4( pow( clr.rgb, vec3(inverse_gamma) ) ,clr.a);
}


-- Post.Vertex

#version 420 core
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;

uniform float scale = 1.0;
uniform vec3 translate = vec3(0,0,0);

out vec2 tex;

void main()
{
	tex = inTexCoord;
	gl_Position = vec4((inVertex.xyz*scale)+translate,1);
}

-- Post.Fragment

#version 420 core
in vec2 tex;
out vec4 fragColour;
uniform sampler2D src_clr;
uniform sampler2D src_dph;

void main()
{
	vec4 clr = texture(src_clr, tex.st);
	vec4 dph = texture(src_dph, tex.st);
	fragColour = vec4(dph.rrr * clr.rgb, clr.a);
}

-- Post.HDR.Luminance.Fragment

#version 420 core

//http://www.cis.rit.edu/people/faculty/ferwerda/publications/sig02_paper.pdf
// equation 1
in vec2 tex;
out vec4 fragColour;
uniform sampler2D src;

void main()
{
	vec4 clr = texture(src, tex.st);
	float lum = clr.r*0.3 + clr.g*0.59 + clr.b*0.11;
	float log_lum = log(0.01 + lum);
	fragColour = vec4(log_lum, log_lum, log_lum, log_lum);
}

-- Post.HDR.VisibleRange.Fragment

#version 420 core
in vec2 tex;
out vec4 fragColour;

uniform sampler2D src;
uniform sampler2D avg_lum;


uniform float smallest_pure_white = 1.0f; //smallest pure white luminance
uniform float key = 0.18;
uniform bool limited_range = false;

void main()
{
	vec4 clr = textureLod(src, tex.st,0);
	float lum = clr.r*0.3 + clr.g*0.59 + clr.b*0.11;

	float average_lug_luminance = exp(textureLod(avg_lum, tex.st, 9).r);	
	float scaled_luminance = (key/average_lug_luminance) * lum;
	
	// this is all from the below
	//http://www.cis.rit.edu/people/faculty/ferwerda/publications/sig02_paper.pdf
	// equation 3
	float ranged_luminance = 0;
	if ( ! limited_range ){
		ranged_luminance = scaled_luminance/(1+scaled_luminance);
	}
	// or 
	// equation 4
	else{
		ranged_luminance = ((scaled_luminance)*(1 + scaled_luminance/(smallest_pure_white*smallest_pure_white)))/(1+scaled_luminance);
	}
	
	//test- - - - - - - - - - - - - - - - - - - - 
	//fragColour = vec4(average_lug_luminance, average_lug_luminance, average_lug_luminance, clr.a);
	//fragColour = vec4(ranged_luminance, ranged_luminance, ranged_luminance, clr.a);
	//fragColour = vec4(textureLod(avg_lum, tex.st, 9).rgb, clr.a);	
	//- - - - - - - - - - - - - - - - - - - - - - 

	fragColour = vec4( pow( clr.rgb * (ranged_luminance / lum), vec3(1/2.2) ), clr.a);
	
}