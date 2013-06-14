#include <platform.h>
#include <debug_print.h>
#include <wrapper_gl.hpp>
#include <shader_gl.hpp>
#include <transforms.h>


/*
simple mipmap example
*/

#define SCALE 100.0f
static const float vertices[] = {
	//0
	-1.0f*SCALE, 0.0f, -1.0f*SCALE, //P
	-1.0f*SCALE, -1.0f*SCALE,		//T
	 //1
	 1.0f*SCALE,  0.0f, -1.0f*SCALE,//P
	 1.0f*SCALE, -1.0f*SCALE,		//T
	 //2
	 1.0f*SCALE, 0.0f, 1.0f*SCALE,	//P
	 1.0f*SCALE,  1.0f*SCALE,		//T
	 //3
	-1.0f*SCALE,  0.0f, 1.0f*SCALE, //P
	-1.0f*SCALE,  1.0f*SCALE		//T
};

static unsigned char indices[] = {	0, 1, 3, 2 };

static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
layout(location = 2) in vec3 inNormal;\n\
layout(location = 3) in vec2 inTC;\n\
uniform mat4		mvp;\n\
out vec3 colour;\n\
out vec2 tc;\n\
void main()\n\
{\n\
    colour = inColour;\n\
	tc = inTC;\
	gl_Position = mvp * vec4(inVertex.xyz,1.0);\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
out vec4 fragColour;\n\
in vec2 tc;\n\
uniform sampler2D tex;\
void main()\n\
{\n\
    fragColour = texture2D(tex, tc.st);\n\
	//fragColour = vec4(1,1,1, 1);\n\
}\n\
";

#define DIM 16 //if you are going to change this remember to 
				//generate mipmaps for all levels of 4th texture
				//either automatically or manually

#define WIDTH DIM
#define HEIGHT DIM
#define MAX_TEXTURES 5

//static variables 
CPlatform* pPlatform = 0;
unsigned int vao=0, eab=0, ab=0, tb=0;	

CShaderProgram program;
int utex = 0, uMVP = 0;
CMatrix44 projection,camera_rotation, mvp;
Transform transform(10);

int activeTexture = 0;
WRender::Texture::SObject tex[MAX_TEXTURES];
//make sure to keep the order the same in here
//otherwise it'll mess with the debug print below
//min filters are every 4th element
const char* sFilters[] = {"NEAREST",
	"LINEAR",
	"NEAREST_MIPMAP_NEAREST",
	"LINEAR_MIPMAP_NEAREST",
	"NEAREST_MIPMAP_LINEAR",
	"LINEAR_MIPMAP_LINEAR"};

//You can change them if you want, but you'll have to chnage the debug print out
WRender::Texture::SParam param[] ={
	//tex 0
	{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST},
	{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
	{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
	{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	// tex 1
	{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR_MIPMAP_NEAREST},
	{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
	{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
	{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	// tex 2
	{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST_MIPMAP_LINEAR},
	{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
	{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
	{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	// tex 3
	{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR_MIPMAP_LINEAR},
	{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
	{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
	{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	// tex 4
	{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR_MIPMAP_LINEAR},
	{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
	{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
	{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
};

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"mipmap example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	unsigned int newWidth = 0;
	unsigned int newHeight = 0;
	int black=0;
	//these will be alloced and filled with the colours their name suggests
	unsigned char * pTexture = 0;
	unsigned char * pTextureRed = 0;
	unsigned char * pTextureGreen = 0;
	unsigned char * pTextureBlue = 0;
	unsigned char * pTextureWhite = 0;

	WRender::Texture::SDescriptor desc = {WRender::Texture::TEX_2D, WRender::Texture::RGB8, WIDTH, HEIGHT, 0, 0, WRender::Texture::GEN_MIPMAP};

	CShader vertexShader(CShader::VERT, &pVertexShader, 1);
	CShader fragmentShader(CShader::FRAG, &pFragmentShader, 1);
	
	// - - - - - - - - - - 
	// create textures
	//in the case of a 16  by 16 texture this will fill in all levels
	 //1x1 is the msallest level for a mipmap
	pTexture = new unsigned char[WIDTH * HEIGHT * 3];
	for(unsigned int h=0;h<HEIGHT;h++)
	{
		for(unsigned int w=0;w<WIDTH;w++)
		{
			if((w > WIDTH/2 && h< HEIGHT/2) || (w < WIDTH/2 && h> HEIGHT/2))
				black = 0;
			else
				black = 1;

			pTexture[(h*WIDTH*3)+(w*3)+0] = (black)?0:255;	//R
			pTexture[(h*WIDTH*3)+(w*3)+1] = (black)?0:255;	//G
			pTexture[(h*WIDTH*3)+(w*3)+2] = (black)?0:255;	//B
		}	
	}
	newWidth = WIDTH/2;
	newHeight = HEIGHT/2;
	pTextureRed = new unsigned char[(newWidth) * (newHeight) * 3];
	for(unsigned int h=0;h<newHeight;h++)
		for(unsigned int w=0;w<newWidth;w++)
		{
			pTextureRed[(h*newWidth*3)+(w*3)+0] = 255;//R
			pTextureRed[(h*newWidth*3)+(w*3)+1] = pTextureRed[(h*newWidth*3)+(w*3)+2] = 0; //GB
		}

	newWidth = newWidth/2;
	newHeight = newHeight/2;
	pTextureGreen = new unsigned char[(newWidth) * (newHeight) * 3];
	for(unsigned int h=0;h<newHeight;h++)
		for(unsigned int w=0;w<newWidth;w++)
		{
			pTextureGreen[(h*newWidth*3)+(w*3)+1] = 255;//G
			pTextureGreen[(h*newWidth*3)+(w*3)+0] = pTextureGreen[(h*newWidth*3)+(w*3)+2] = 0; //RB
		}
	newWidth = newWidth/2;
	newHeight = newHeight/2;
	pTextureBlue = new unsigned char[(newWidth) * (newHeight) * 3];
	for(unsigned int h=0;h<newHeight;h++)
		for(unsigned int w=0;w<newWidth;w++)
		{
			pTextureBlue[(h*newWidth*3)+(w*3)+2] = 255;					//B
			pTextureBlue[(h*newWidth*3)+(w*3)+0] = pTextureBlue[(h*newWidth*3)+(w*3)+1] = 0; //RG
		}

	newWidth = newWidth/2;
	newHeight = newHeight/2;
	pTextureWhite = new unsigned char[(newWidth) * (newHeight) * 3];
	memset(pTextureWhite, 255, (newWidth) * (newHeight) * 3);

	WRender::Pixel::Data data = {WRender::Pixel::RGB, WRender::Pixel::UCHAR, 0, (void*)pTexture};
	WRender::ActiveTexture(WRender::Texture::UNIT_0);
	//tex 0
	if(WRender::CreateBaseTexture(tex[0], desc))
	{		
		WRender::UpdateTextureData(tex[0], data);
		WRender::SetTextureParams(tex[0], param, 4);		
	}
	//tex 1
	if(WRender::CreateBaseTextureData(tex[1], desc, data))
		WRender::SetTextureParams(tex[1], param+4, 4);		
	//tex 2
	if(WRender::CreateBaseTextureData(tex[2], desc, data))
		WRender::SetTextureParams(tex[2], param+8, 4);		
	//tex 3
	if(WRender::CreateBaseTextureData(tex[3], desc, data))
		WRender::SetTextureParams(tex[3], param+12, 4);		
	//tex 4
	//for the final one use a custom mipmap
	//desc.genMipMap = WRender::Texture::GEN_MIPMAP; //if you are going to set all levels of the mipmap manually then setting this to 
	desc.genMipMap = WRender::Texture::DONT_GEN_MIPMAP; //if you are going to set all levels of the mipmap manually then setting this to 
	 //false is Ok, if you are not and you only wish to override some levels of the mipmap, set it to true and then
	 //
	 //in the case of a 16  by 16 texture this will fill in all levels
	 //1x1 is the msallest level for a mipmap
	if(WRender::CreateBaseTextureData(tex[4], desc, data))
	{
		desc.genMipMap = WRender::Texture::DONT_GEN_MIPMAP;

		data.pData = (void*)pTextureRed;
		desc.w = WIDTH/2;
		desc.h = HEIGHT/2;
		data.level = 1;
		WRender::CreateTextureData(tex[4], desc, data);

		data.pData = (void*)pTextureGreen;
		desc.w = WIDTH/4;
		desc.h = HEIGHT/4;
		data.level = 2;
		WRender::CreateTextureData(tex[4], desc, data);		
		
		data.pData = (void*)pTextureBlue;
		desc.w = WIDTH/8;
		desc.h = HEIGHT/8;
		data.level = 3;
		WRender::CreateTextureData(tex[4], desc, data);		
		
		data.pData = (void*)pTextureWhite;
		desc.w = WIDTH/16;
		desc.h = HEIGHT/16;
		data.level = 4;
		WRender::CreateTextureData(tex[4], desc, data);

		WRender::SetTextureParams(tex[4], param+16, 4);		
	}
	delete [] pTexture;
	delete [] pTextureRed;
	delete [] pTextureGreen;
	delete [] pTextureBlue;
	delete [] pTextureWhite;

	// - - - - - - - - - - 
	//setup the shaders

	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();

	utex = program.GetUniformLocation("tex");
	uMVP = program.GetUniformLocation("mvp");

	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*5, 0, 0},
		//{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*11, sizeof(float)*3, 0},
		//{ab, 2, 3, WRender::FLOAT, 0, sizeof(float)*11, sizeof(float)*6, 0},
		{ab, 3, 2, WRender::FLOAT, 0, sizeof(float)*5, sizeof(float)*3, 0}
	};
	WRender::SetAttributeFormat( va, 2, 0);

	Transform::CreateProjectionMatrix(projection, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f*SCALE + 10.0f);	

	program.Start();
	program.SetTextureUnit(utex, 0);
	WRender::EnableCulling(false);
	WRender::BindTexture(tex[activeTexture]);
	d_printf("Texture Index %d, id %d\n",activeTexture,tex[activeTexture].handle);
}


void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	CVec3df cameraPosition(0, 0.0f, -10.0f);
	pPlatform->Tick();			
	if(pPlatform->GetKeyboard().keys[KB_SPACE].IsToggledPress())
	{
		if(++activeTexture >= MAX_TEXTURES)
			activeTexture = 0;
		WRender::BindTexture(tex[activeTexture]);
		d_printf("Texture Index %d, id %d\n",activeTexture,tex[activeTexture].handle);			
		d_printf("Texture Min Filter %s \n",sFilters[param[activeTexture*4].param - WRender::Texture::NEAREST]);			

	}
	transform.Push();
	CMatrix44 rotLat,rotLong;
	rotLat.Rotate(25.0f, 1, 0, 0);
	rotLong.Rotate(25.0f, 0, 1, 0);
	transform.Translate(cameraPosition);
	transform.ApplyTransform(rotLat);
	transform.ApplyTransform(rotLong);		
	mvp = projection * transform.GetCurrentMatrix();
	program.SetMtx44(uMVP, mvp.data);

	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);		
	//swap buffers
	pPlatform->UpdateBuffers();
	transform.Pop();
}

void CleanUp(void)
{	
	int i=0;
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);	
	program.CleanUp();
	for(i=0;i<MAX_TEXTURES;i++)
		WRender::DeleteTexture(tex[i]);
}