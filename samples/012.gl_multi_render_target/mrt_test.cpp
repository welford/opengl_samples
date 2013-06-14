#include "platform.h"
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"
#include "transforms.h"
#include "glsw.h"

/*
Simple Multi Render Buffer (MRT) example
render the RGB to 3 different textures, very wasteful, 
but you'll get the idea.

//more examples
http://www.opengl.org/wiki/Framebuffer_Object_Examples
*/

//PTT
static const float sqVertices[] = {
	//0
	 0,0,0,
	 0,0,
	 //1
	 1.0f,0,0,
	 1.0f,0,
	 //2
	 1.0f,1.0f,0,
	 1.0f,1.0f,
	 //3
	 0,1.0f,0,
	 0,1.0f
};
static unsigned char sqIndices[] = {	0, 1, 3, 2 };
#define WIDTH_HEIGHT 128

//static variables 
CPlatform* pPlatform = 0;

WRender::FrameBuffer::SObject fbo;	
WRender::Texture::SObject texR;
WRender::Texture::SObject texG;
WRender::Texture::SObject texB;
WRender::Texture::SObject texD;
unsigned int vao=0, ab=0;	
unsigned int sqVao=0, sqEab=0, sqAb=0;	

CShaderProgram program[2];

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"MRT example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{	
	const char* pV = 0;
	const char* pF = 0;
	float vertices[3] = {0,0,0};
	
	glswInit();
	glswSetPath("../resources/", ".glsl");

	//setup the textures
	WRender::Texture::SDescriptor desc = {WRender::Texture::TEX_2D, WRender::Texture::RGB8, WIDTH_HEIGHT, WIDTH_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	//WRender::Texture::SDescriptor descHalf = {WRender::Texture::TEX_2D, WRender::Texture::RGB8, WIDTH_HEIGHT/2, WIDTH_HEIGHT/2, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SDescriptor descDepth = {WRender::Texture::TEX_2D, WRender::Texture::DEPTH_STENCIL, WIDTH_HEIGHT, WIDTH_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SParam param[] ={	
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	};

	WRender::CreateBaseTexture(texR, desc);
	WRender::CreateBaseTexture(texG, desc);
	WRender::CreateBaseTexture(texB, desc);
	//WRender::CreateBaseTexture(texB, descHalf);
	WRender::CreateBaseTexture(texD, descDepth);

	WRender::SetTextureParams(texR,param,4);
	WRender::SetTextureParams(texG,param,4);
	WRender::SetTextureParams(texB,param,4);
	WRender::SetTextureParams(texD,param,4);

	//setup Frame Buffer
	WRender::CreateFrameBuffer(fbo);
	WRender::AddTextureRenderBuffer(fbo, texR, WRender::ATT_CLR0, 0);
	WRender::AddTextureRenderBuffer(fbo, texG, WRender::ATT_CLR1, 0);
	WRender::AddTextureRenderBuffer(fbo, texB, WRender::ATT_CLR2, 0);
	WRender::AddTextureRenderBuffer(fbo, texD, WRender::ATT_DEPTH_AND_STENCIL, 0);
	WRender::CheckFrameBuffer(fbo);

	//setup shaders	
	pV = glswGetShaders("shaders.MRT.Stage1.Vertex");
	pF = glswGetShaders("shaders.MRT.Stage1.Fragment");
	CShader vertexShader(CShader::VERT, &pV, 1);
	CShader fragmentShader(CShader::FRAG, &pF, 1);

	pV = glswGetShaders("shaders.MRT.Stage2.Vertex");
	pF = glswGetShaders("shaders.MRT.Stage2.Fragment");
	CShader vertexShaderStage2(CShader::VERT, &pV, 1);
	CShader fragmentShaderStage2(CShader::FRAG, &pF, 1);


	program[0].Initialise();
	program[0].AddShader(&vertexShader);
	program[0].AddShader(&fragmentShader);
	//program[0].SetMRTOutputLocation("outFragColour[0]", 3);
	program[0].Link();
	//int a = program[0].GetMRTOutputLocation("outFragColour[0]");
	//int b = program[0].GetMRTOutputLocation("outFragColour[1]");

	program[1].Initialise();
	program[1].AddShader(&vertexShaderStage2);
	program[1].AddShader(&fragmentShaderStage2);
	program[1].Link();
	//quickly set the uniforms
	program[1].Start();
	program[1].SetTextureUnit("mrt",0);
	program[1].Stop();

	// setup vertex buffers etc	
	//point
	vao = WRender::CreateVertexArrayObject();
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);		
	WRender::BindVertexArrayObject(vao);
	WRender::VertexAttribute va[1] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},	//vertices		
	};
	WRender::SetAttributeFormat( va, 1, 0);
	WRender::UnbindVertexArrayObject();//needed

	//square
	sqVao = WRender::CreateVertexArrayObject();	
	sqEab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(sqIndices), sqIndices);
	sqAb = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(sqVertices), sqVertices);
	WRender::BindVertexArrayObject(sqVao);
	WRender::BindBuffer(WRender::ELEMENTS, sqEab);
	WRender::VertexAttribute sqVa[2] = {
		{sqAb, 0, 3, WRender::FLOAT, 0, sizeof(float)*5, 0, 0},				//vertices
		{sqAb, 1, 2, WRender::FLOAT, 0, sizeof(float)*5, sizeof(float)*3, 0},	//texture coordinates
	};
	WRender::SetAttributeFormat( sqVa, 2, 0);
	WRender::UnbindVertexArrayObject();

	//variables for camera 
	WRender::EnableDepthTest(true);
}

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	pPlatform->Tick();

	//First stage Render to the MRTs
	WRender::BindVertexArrayObject(vao);
	program[0].Start();
	WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo);
	WRender::DrawBuffers buffers[] = {WRender::DBS_CLR_ATT_0, WRender::DBS_CLR_ATT_1, WRender::DBS_CLR_ATT_2};
	WRender::SetDrawBuffers(3, buffers);
	WRender::SetClearColour(1,1,1,1);
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::SetupViewport(0, 0, WIDTH_HEIGHT, WIDTH_HEIGHT);				
	WRender::DrawArray(WRender::POINTS, 1, 0);
	WRender::UnbindFrameBuffer(WRender::FrameBuffer::DRAW);
	WRender::SetDrawBuffer(WRender::DB_BACK);

	//Second stage Render the textures bound to the MRTS to the screen
	WRender::SetClearColour(0,0,0,0);
	WRender::SetupViewport(0, 0, 640, 640);
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::BindVertexArrayObject(sqVao);		
	program[1].Start();
	float tranR[3] = {-1.0f, -1.0f, 0};
	float tranG[3] = { 0.0f, -1.0f, 0};
	float tranB[3] = {-1.0f, 0.0f, 0};
	float tranD[3] = { 0.0f, 0.0f, 0};

	WRender::BindTexture(texR,WRender::Texture::UNIT_0);
	program[1].SetVec3("translate",tranR);		
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);
	WRender::BindTexture(texG,WRender::Texture::UNIT_0);
	program[1].SetVec3("translate",tranG);
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);
	WRender::BindTexture(texB,WRender::Texture::UNIT_0);
	program[1].SetVec3("translate",tranB);
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);
	WRender::BindTexture(texD,WRender::Texture::UNIT_0);
	program[1].SetVec3("translate",tranD);
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);

	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);

	WRender::DeleteVertexArrayObject(sqVao);
	WRender::DeleteBuffer(sqAb);
	WRender::DeleteBuffer(sqEab);	

	WRender::DeleteTexture(texR);
	WRender::DeleteTexture(texG);
	WRender::DeleteTexture(texB);
	WRender::DeleteTexture(texD);

	WRender::DeleteFrameBuffer(fbo);

	program[0].CleanUp();
	program[1].CleanUp();
	glswShutdown();
}