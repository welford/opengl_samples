Some hopefully simple openGL examples

They aren't commented very well, but hopefully give a stripped down insite into how to do various things in OpenGL 3/4

A few notes:
	-All samples reply on glew which i have included statically in glew_static
	-All but one project relies on "platform" to create the windows environment
		I have tried to make it as easy as possible for this to run on different OSs	
	-The early examples don't use my OpenGL wrapper, they do use my shader wrapper though - it should be very simple to follow

the helper libraries are:
-glew : static include of glew
-platform : handle window creation
-utils : contains the source for the shader wrapper (C/C++ variants), opengl wrapper, 
		 creating shapes, a simple matrix stack, 
	
The exaples so far are:

01. gl_simple_tri : creates a simple coloured triangle
01(a). gl_no_external_libs : and as 01 but doesn't link to the utils or platform projects, contails all files internally
02. gl_simple_uniform : uses a shader uniform to edit the position and colour of a triangle
03. gl_simple_wrapper : using my openGL wrapper
04. gl_shader_wrangler: using Phillip Rideout's Shader Wranger : http://prideout.net/blog/?p=11
05. gl_clipspace : draws some quads in clipspace
06. gl_simple_texture : creates a simple textured quad and shows various effects of texture parameters
07. gl_uniform_buffer_object: shows how to setup and use a UBO
08. gl_transforms: doing old opengl like push and pop operations on transforms
09. gl_camera: making a very simple camera
10. gl_mipmap : sets up a texture and lets you switch between various mipmap settings
11. gl_mapped_buffer: example of mapping an openGL buffer so that you cna write to it directly
12. gl_multi_render_target: sets up and renders to a MRT with 4 buffers attached (the component colours RGB and depth are rendered out individually to textures)
13. gl_mrt_2: same as a bove but includes a Render Buffer Object also being attached
14. gl_shapes: creates vertices for shapes and draws them (sphere, torus, frustum, cone, Cylinder, Plane)
15. gl_shadow: very simple example of how to do shadow mapping, no other lighting is performed.
16. gl_shadows_multi_fbo: brute force exampe of rendering multiple shadows using multiple FBOs, will do some layered rendering later 
17. gl_geometry_shader: contains 3 examples of geometry shaders 1. pass through, 2. adding extra addtributes 3. Duplicating geometry
18. gl_gamma_correction : g,c,b keys control Gamma, Contrast, and Brightness respectively 
19. gl_postprocess_template : Renders a sphere to a FBO with a depth and RGB texture bound. Then uses the depth and RGB textures in a post process render.
20. gl_high_dynamic_range: not quite finished
21. gl_cubemap: setting up and rendering cubemaps
22. gl_cubemap_reflections: improves on the previous lesson, adds reflections

I will add more as i go along.