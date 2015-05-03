#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <shapes.h>
#include <math.h>
#include "transforms.h"
#include "glsw.h"

//PTT
static const float sqVertices[] = {
	//0
	 -1.0f, -1.0f, 0,
	 0,0,
	 //1
	 1.0f, -1.0f, 0,
	 1.0f,0,
	 //2
	 1.0f, 1.0f, 0,
	 1.0f,1.0f,
	 //3
	 -1.0f, 1.0f, 0,
	 0,1.0f
};
static unsigned char sqIndices[] = {	0, 1, 3, 2 };


/*
*/
typedef struct {
	CMatrix44 mvp;
	CMatrix44 proj; 
	CMatrix44 mv;
	float nrm[16]; //3x3 matrix	
} Transforms;

//static vars 
CPlatform* pPlatform = 0;
Transform transform(10);
float latitude = 0.0f, longitude = 0.0f;
Transforms transforms;

//shaders
CShaderProgram program[2]; //0 normal,  1 post

//buffer for drawing
unsigned int ubo=0;	
unsigned int vaoSphere=0, abSphere=0;	
unsigned int sqVao=0, sqEab=0, sqAb=0;	
//amount of vertices to draw
unsigned int nSphereVertices;

//FBO to render to
#define WIDTH_HEIGHT 512
WRender::FrameBuffer::SObject fbo;	
WRender::Texture::SObject texRGB;
WRender::Texture::SObject texD;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"Hello", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);	
	Setup(pPlatform);
	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	unsigned int got = 0;
	const char *pVertStr[3] = {0,0,0}, *pFragStr = 0;

	unsigned int nSphereFloats;
	float *pSphereVertices = 0;
	unsigned int iterations = 6;	
	
	int nFacets = 0;

	glswInit();
	glswSetPath("../resources/", ".glsl");
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	
	got = glswGetShadersAlt("shaders.Version+shaders.Shared+shaders.FlatShading.Vertex", pVertStr, 3);
	pFragStr = glswGetShaders("shaders.Version+shaders.FlatShading.Fragment");
	CShader vertexShader(CShader::VERT, pVertStr, got);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);

	pVertStr[0] = glswGetShaders("shaders.Version+shaders.Post.Vertex");
	pFragStr = glswGetShaders("shaders.Version+shaders.Post.Fragment");
	CShader vertexShaderStage2(CShader::VERT, &pVertStr[0], 1);
	CShader fragmentShaderStage2(CShader::FRAG, &pFragStr, 1);

	// - - - - - - - - - - 
	//setup the shaders
	program[0].Initialise();
	program[0].AddShader(&vertexShader);
	program[0].AddShader(&fragmentShader);
	program[0].Link();

	program[1].Initialise();
	program[1].AddShader(&vertexShaderStage2);
	program[1].AddShader(&fragmentShaderStage2);
	program[1].Link();
	program[1].Start();
	program[1].SetTextureUnit("src_clr",0);
	program[1].SetTextureUnit("src_dph",1);
	program[1].Stop();

	// - - - - - - - - - - 
	//setup the textures
	WRender::Texture::SDescriptor desc = {WRender::Texture::TEX_2D, WRender::Texture::RGB8, WIDTH_HEIGHT, WIDTH_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SDescriptor descDepth = {WRender::Texture::TEX_2D, WRender::Texture::DEPTH_COMPONENT, WIDTH_HEIGHT, WIDTH_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SParam param[] ={	
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},		
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},
	};
	WRender::CreateBaseTexture(texRGB, desc);
	WRender::SetTextureParams(texRGB,param,4);

	WRender::CreateBaseTexture(texD, descDepth);
	WRender::SetTextureParams(texD,param,4);
	//setup Frame Buffer
	WRender::CreateFrameBuffer(fbo);
	WRender::AddTextureRenderBuffer(fbo, texRGB, WRender::ATT_CLR0, 0);
	WRender::AddTextureRenderBuffer(fbo, texD, WRender::ATT_DEPTH, 0);
	WRender::CheckFrameBuffer(fbo);

	// - - - - - - - - - - 
	//set up shapes		
	//SPHERE
	nSphereVertices = unsigned int(powl(4.0,long double(iterations)))*8*3;
	nSphereFloats = nSphereVertices*3;
	pSphereVertices = new float[nSphereFloats];
	nFacets = CreateNSphere(pSphereVertices, iterations);
	vaoSphere = WRender::CreateVertexArrayObject();
	abSphere = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nSphereFloats, pSphereVertices);	

	WRender::BindVertexArrayObject(vaoSphere);
	WRender::VertexAttribute vaSphere[1] = {
		{abSphere, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0}
	};
	WRender::SetAttributeFormat( vaSphere, 1, 0);
	delete[] pSphereVertices;

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

	//ubo for cameras etc
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
}

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	CVec3df cameraPosition(0, 0.0f, -2.1f);
	pPlatform->Tick();	

	//first pass to texture
	program[0].Start();
	WRender::SetClearColour(0.0,0,0,0);
	WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo);
	WRender::SetupViewport(0, 0, WIDTH_HEIGHT, WIDTH_HEIGHT);
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::EnableDepthTest(true);
	transform.Push();			
	{
		CMatrix44 rotLat,rotLong;
		rotLat.Rotate(latitude, 1, 0, 0);
		rotLong.Rotate(longitude, 0, 1, 0);

		transform.Translate(cameraPosition);
		transform.ApplyTransform(rotLat);
		transform.ApplyTransform(rotLong);			
			
		transform.Push();					
		{
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			//Sphere
			WRender::BindVertexArrayObject(vaoSphere);
			WRender::DrawArray(WRender::TRIANGLES, nSphereVertices, 0);
		}
		transform.Pop();
	}
	transform.Pop();		
	WRender::UnbindFrameBuffer(WRender::FrameBuffer::DRAW);
	WRender::SetDrawBuffer(WRender::DB_BACK);

	//second pass
	//texture to screen
	WRender::SetClearColour(1.0,0,0,0);
	WRender::SetupViewport(0, 0, 640, 640);
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::BindVertexArrayObject(sqVao);		
	program[1].Start();
	//float tranR[3] = {0.0f, -1.0f, 0};	
	WRender::BindTexture(texRGB,WRender::Texture::UNIT_0);
	WRender::BindTexture(texD,WRender::Texture::UNIT_1);
	//program[1].SetVec3("translate",tranR);		
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);

	pPlatform->UpdateBuffers();
	if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed())
		latitude += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed())
		latitude -= 90.0f * pPlatform->GetDT();
		
	if(pPlatform->GetKeyboard().keys[KB_LEFT].IsPressed())//l
		longitude += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_RIGHT].IsPressed())//r
		longitude -= 90.0f * pPlatform->GetDT();
}

void CleanUp(void)
{	
	WRender::DeleteVertexArrayObject(vaoSphere);
	WRender::DeleteBuffer(ubo);
	WRender::DeleteBuffer(abSphere);
	
	WRender::DeleteVertexArrayObject(sqVao);
	WRender::DeleteBuffer(sqAb);
	WRender::DeleteBuffer(sqEab);	

	WRender::DeleteTexture(texRGB);
	WRender::DeleteTexture(texD);
	WRender::DeleteFrameBuffer(fbo);

	program[0].CleanUp();
	program[1].CleanUp();
	glswShutdown();
}