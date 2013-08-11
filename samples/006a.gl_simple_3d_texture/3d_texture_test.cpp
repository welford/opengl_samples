#include <platform.h>
#include <debug_print.h>
#include <GL/glew.h>
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"

/*
simple texture example
*/

//FMT
//PCCCNNN
static const float vertices[] = {
	//0
	-0.5f, -0.5f, 0.0f, //P
	-0.5f, -0.5f,		///TX
	 //1
	 0.5f, -0.5f, 0.0f, //P
	 1.5f, -0.5f,		///TX
	 //2
	 +0.5f, 0.5f, 0.0f, //P
	 1.5f,  1.5f,		///TX
	 //3
	-0.5f,  0.5f, 0.0f, //P
	-0.5f,  1.5f		///TX
};

static unsigned char indices[] = {	0, 1, 3, 2 };

static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 3) in vec2 inTC;\n\
out vec2 tc;\n\
void main()\n\
{\n\
	tc = inTC;\
	gl_Position = vec4(inVertex.xyz,1.0);\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
out vec4 fragColour;\n\
in vec2 tc;\n\
uniform float layer = 0.0;\
layout(binding=4) uniform sampler3D tex;\
void main()\n\
{\n\
	fragColour = texture(tex, vec3(tc.st,layer) );\n\
}\n\
";
//
#define WIDTH 16
#define HEIGHT 16
#define DEPTH 16
#define MAX_TEXTURES 5

//statis vars
CPlatform* pPlatform = 0;	
unsigned int vao=0, eab=0, ab=0;	
int activeTexture = 0;
WRender::Texture::SObject tex[MAX_TEXTURES];

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"3d texture example, phasersonkill.com ", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

CShaderProgram program;

void Setup(CPlatform * const  pPlatform)
{	
	int utex = 0;
	unsigned char * pTexture = 0;
	WRender::Texture::SDescriptor desc = {WRender::Texture::TEX_3D, WRender::Texture::RGB8, WIDTH, HEIGHT, DEPTH, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SParam param[] ={
		//tex 0
		{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
		// tex 1
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
		// tex 2
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
		// tex 3
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::CLAMP_TO_EDGE},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
		// tex 4
		{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::WRAP_S, WRender::Texture::CLAMP_TO_EDGE},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
	};
	CShader vertexShader(CShader::VERT, &pVertexShader, 1);
	CShader fragmentShader(CShader::FRAG, &pFragmentShader, 1);	
	WRender::Pixel::Data pixelData = {WRender::Pixel::RGB, WRender::Pixel::UCHAR, 0, 0};

	// - - - - - - - - - - 
	// create textures
	pTexture = new unsigned char[WIDTH * HEIGHT * DEPTH * 3];
	for(unsigned int d=0;d<DEPTH;d++)
		for(unsigned int h=0;h<HEIGHT;h++)
			for(unsigned int w=0;w<WIDTH;w++)			
			{
				int depth_index = d*HEIGHT*WIDTH*3;
				int height_index = h*WIDTH*3;
				int width_index = w*3;

				pTexture[depth_index + height_index + width_index + 0] = (255*w)/(WIDTH);			//R
				pTexture[depth_index + height_index + width_index + 1] = (255*h)/(HEIGHT);			//G
				pTexture[depth_index + height_index + width_index + 2] = (255*d)/(DEPTH);//(255*(w+h))/(WIDTH+HEIGHT);	//B
			}	
	pixelData.pData = (void*)pTexture;
	
	WRender::ActiveTexture(WRender::Texture::UNIT_4);
	//tex 0
	if(WRender::CreateBaseTexture(tex[0], desc))
	{		
		WRender::UpdateTextureData(tex[0], pixelData);
		WRender::SetTextureParams(tex[0], param, 4);		
	}
	//tex 1
	if(WRender::CreateBaseTextureData(tex[1], desc, pixelData))
		WRender::SetTextureParams(tex[1], param+4, 4);		
	//tex 2
	if(WRender::CreateBaseTextureData(tex[2], desc, pixelData))
		WRender::SetTextureParams(tex[2], param+8, 4);		

	//tex 3
	if(WRender::CreateBaseTextureData(tex[3], desc, pixelData))
		WRender::SetTextureParams(tex[3], param+12, 4);		

	//tex 4
	if(WRender::CreateBaseTextureData(tex[4], desc, pixelData))
		WRender::SetTextureParams(tex[4], param+16, 4);		
	delete [] pTexture;	

	// - - - - - - - - - - 
	//setup the shaders
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();
	utex = program.GetUniformLocation("tex");

	// - - - - - - - - - - 
	// setup vertex buffers etc
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*5, 0, 0},
		{ab, 3, 2, WRender::FLOAT, 0, sizeof(float)*5, sizeof(float)*3, 0}
	};
	WRender::SetAttributeFormat( va, 2, 0);
	program.Start();
	//program.SetTextureUnit(utex, 4);
	glEnable(GL_CULL_FACE);
	WRender::BindTexture(tex[activeTexture]);	

}
void MainLoop(CPlatform * const  pPlatform)
{
	pPlatform->Tick();	
	if(pPlatform->GetKeyboard().keys[KB_SPACE].IsToggledPress())
	{
		if(++activeTexture >= MAX_TEXTURES)
			activeTexture = 0;
		WRender::BindTexture(tex[activeTexture]);
		d_printf("Texture Index %d, id %d\n",activeTexture,tex[activeTexture].handle);			
	}
	static float layer = 0.0f;
	program.SetFloat( "layer", layer );
	layer += 0.01f;
	if (layer > 1.0){
		layer -= 1.0f;
	}

	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);		
	//swap buffers
	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{
	int i=0;
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);
	for(i=0;i<MAX_TEXTURES;i++)
		WRender::DeleteTexture( tex[i] );	
}