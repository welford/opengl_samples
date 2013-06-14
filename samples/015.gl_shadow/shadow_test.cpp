#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <shapes.h>
#include "transforms.h"
#include "glsw.h"
//#include <gl/glew.h>

/*
For rendering textures to the screen
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

static unsigned char sqIndices[] = {0, 1, 3, 2 };

typedef struct {
	CMatrix44 mvp;
	CMatrix44 proj; 
	CMatrix44 mv;
	float nrm[16]; //3x3 matrix	
} Transforms;

#define TEX_DIMENSIONS 32

CPlatform* pPlatform = 0;
Transform transform(10);
Transform shadow_transform(10);
Transforms transforms;

//Various OpenGL buffers
WRender::FrameBuffer::SObject fbo;
unsigned int ubo=0;						//for holding tranform data
unsigned int abColour=0;	
unsigned int vaoTorus=0, abTorus=0;		//
unsigned int vaoPlane=0, abPlane=0;		//
unsigned int sqVao=0, sqEab=0, sqAb=0;	//
WRender::Texture::SObject depthTexture;

//for the camera
float latitude = 45.0f, longitude = 45.0f;
float lightLatitude = 45.0f, lightLongitude = 0.0f;
float *pLat = &latitude, *pLong = &longitude;
CVec3df cameraPosition(0, 0.0f, -10.0f);
CMatrix44 rotLat,rotLong;

//shaders 
CShaderProgram program;
CShaderProgram textureShader;

int uShadowMtx = 0;
unsigned int nTorusVertices = 0;

void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void DrawGeometry(bool shadowPass);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"shadow example, phaseronkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);	
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	//for the shapes
	unsigned int nTorusFloats;
	float *pTorusVertices = 0;
	float *pPlaneVertices = 0;	
	float colour[] = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f};
	unsigned int torusSegments = 36, torusTubeSegments = 36;

	//for the shaders
	const char *pVertStr[2] = {0,0}, *pFragStr = 0;
	const char *pFragTexStr = 0, *pVertTexStr = 0;

	//fbo stuff
	WRender::Texture::SDescriptor descDepth = {WRender::Texture::TEX_2D, WRender::Texture::DEPTH_COMPONENT, TEX_DIMENSIONS, TEX_DIMENSIONS, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SParam param[] ={	
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::CLAMP_TO_EDGE},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
	};	
	
	glswInit();
	glswSetPath("../resources/", ".glsl");
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	// - - - - - - - - - - 
	//setup the shaders
	// - - - - - - - - - - 

	//normal shader
	glswGetShadersAlt("shaders.Shared+shaders.SingleShadow.Vertex", pVertStr, 2);
	pFragStr = glswGetShaders("shaders.SingleShadow.Fragment");
	CShader vertexShader(CShader::VERT, pVertStr, 2);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);
	
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();
	uShadowMtx = program.GetUniformLocation("shadowMtx");
	program.Start();
	program.SetTextureUnit("shadowMap", 0);
	program.Stop();

	//debug shader for textures in screen space
	pVertTexStr = glswGetShaders("shaders.Dbg.ScreenSpaceTexture.Vertex");
	pFragTexStr = glswGetShaders("shaders.Dbg.ScreenSpaceTexture.Fragment");
	CShader vTexShader(CShader::VERT, &pVertTexStr, 1);
	CShader fTexShader(CShader::FRAG, &pFragTexStr, 1);	

	textureShader.Initialise();
	textureShader.AddShader(&vTexShader);
	textureShader.AddShader(&fTexShader);
	textureShader.Link();
	textureShader.Start();
	textureShader.SetTextureUnit("texture",0);
	textureShader.Stop();

	// - - - - - - - - - - 
	//set up shapes
	// - - - - - - - - - - 
	//shared colours
	abColour = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(colour), colour);

	//Torus
	nTorusVertices = 2*torusTubeSegments*torusSegments*3;
	nTorusFloats = nTorusVertices*3;
	pTorusVertices = new float[nTorusFloats];
	CreateTorus(pTorusVertices, torusSegments, 3.0f, torusTubeSegments, 1.0f);	
	vaoTorus = WRender::CreateVertexArrayObject();
	abTorus = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nTorusFloats, pTorusVertices);	

	WRender::BindVertexArrayObject(vaoTorus);
	WRender::VertexAttribute vaTorus[2] = {
		{abTorus, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*9, sizeof(float)*6, 1},
	};
	WRender::SetAttributeFormat( vaTorus, 2, 0);
	delete[] pTorusVertices;

	//Plane
	pPlaneVertices = new float[4*3*3];
	CreatePlane(pPlaneVertices, 20.0f, 20.0f);
	vaoPlane = WRender::CreateVertexArrayObject();
	abPlane = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*4*3*3, pPlaneVertices);	

	WRender::BindVertexArrayObject(vaoPlane);
	WRender::VertexAttribute vaFrustum[2] = {
		{abPlane, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*9, sizeof(float)*3, 1},
	};
	WRender::SetAttributeFormat( vaFrustum, 2, 0);
	delete[] pPlaneVertices;

	//for screen aligned texture
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

	// - - - - - - - - - - 
	//ubo for cameras etc
	// - - - - - - - - - - 
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);

	//create the texture and FBO for rendering to when drawing 
	//during shadow stage
	WRender::CreateBaseTexture(depthTexture, descDepth);	
	WRender::SetTextureParams(depthTexture,param,4);

	WRender::CreateFrameBuffer(fbo);
	WRender::AddTextureRenderBuffer(fbo, depthTexture, WRender::ATT_DEPTH, 0);
	WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo);
	WRender::SetDrawBuffer(WRender::DB_NONE);
	WRender::CheckFrameBuffer(fbo);

	//set the projection matrix
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
	WRender::BindTexture(depthTexture,WRender::Texture::UNIT_0);	
}

void MainLoop(CPlatform * const  pPlatform)
{
	float tranD[3] = {0.0f, -1.0f, 0};				
	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::EnableDepthTest(true);		
	program.Start();

	//Draw from light perspective
	//glPolygonOffset( 2.5f, 25.0f );
	//glEnable( GL_POLYGON_OFFSET_FILL);
	transform.Push();			
	{
		WRender::SetupViewport(0, 0, TEX_DIMENSIONS, TEX_DIMENSIONS);	//adjust viewport
		WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo);		//set fbo 
		WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);				//clear draw buffers (must be called after we set the fbo)

		rotLat.Identity();	rotLat.Rotate(lightLatitude, 1, 0, 0);
		rotLong.Identity();	rotLong.Rotate(lightLongitude, 0, 1, 0);

		transform.Translate(cameraPosition);			
		transform.ApplyTransform(rotLat);
		transform.ApplyTransform(rotLong);

		//setup shadow mvp
			
		shadow_transform.Identity();
		shadow_transform.Translate(0.5f, 0.5f, 0.5f);
		shadow_transform.Scale(0.5f,0.5f,0.5f);
		shadow_transform.ApplyTransform(transforms.proj);
		shadow_transform.ApplyTransform(transform.GetCurrentMatrix());
		DrawGeometry(true);

		WRender::UnbindFrameBuffer(WRender::FrameBuffer::DRAW); //reset draw buffer
		WRender::SetupViewport(0, 0, 640, 640);	//reset viewport			
	}
	//glDisable( GL_POLYGON_OFFSET_FILL);
	transform.Pop();		

	//draw scene
	transform.Push();			
	{
		rotLat.Identity();	rotLat.Rotate(latitude, 1, 0, 0);
		rotLong.Identity();	rotLong.Rotate(longitude, 0, 1, 0);

		transform.Translate(cameraPosition);
		transform.ApplyTransform(rotLat);
		transform.ApplyTransform(rotLong);
		DrawGeometry(false);			
	}
	transform.Pop();		

	//render depth texture to screen
	textureShader.Start();
	WRender::BindVertexArrayObject(sqVao);					
	textureShader.SetVec3("translate",tranD);		
	WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);		
		
	//update Keyboard
	pPlatform->UpdateBuffers();
	if(!pPlatform->GetKeyboard().keys[KB_LEFTSHIFT].IsPressed())
	{	pLat = &latitude; pLong = &longitude;}
	else
	{	pLat = &lightLatitude; pLong = &lightLongitude;}

	if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed())
		*pLat += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed())
		*pLat -= 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_LEFT].IsPressed())//l
		*pLong += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_RIGHT].IsPressed())//r
		*pLong -= 90.0f * pPlatform->GetDT();		

}

void DrawGeometry(bool shadowPass)
{
	CMatrix44 rot;
	rot.Rotate(45,1,0,0);

	transform.Push();
	if(shadowPass == false)
		shadow_transform.Push();

	transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
	WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
	if(shadowPass == false)
		program.SetMtx44(uShadowMtx, shadow_transform.GetCurrentMatrix().data);
	//Plane
	WRender::BindVertexArrayObject(vaoPlane);
	WRender::DrawArray(WRender::TRIANGLES, 4*3, 0);

	transform.Pop();
	if(shadowPass == false)
		shadow_transform.Pop();
	
	transform.Push();
	if(shadowPass == false)
		shadow_transform.Push();
	
	transform.ApplyTransform(rot);
	transform.Translate(0, 5.0f, 0);
	if(shadowPass == false)
	{
		shadow_transform.ApplyTransform(rot);
		shadow_transform.Translate(0, 5.0f, 0);
	}
	transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
	WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
	if(shadowPass == false)
		program.SetMtx44(uShadowMtx, shadow_transform.GetCurrentMatrix().data);
	//Torus
	WRender::BindVertexArrayObject(vaoTorus);
	WRender::DrawArray(WRender::TRIANGLES, nTorusVertices, 0);

	transform.Pop();
	if(shadowPass == false)
		shadow_transform.Pop();
}

void CleanUp()
{
	WRender::DeleteVertexArrayObject(vaoTorus);
	WRender::DeleteVertexArrayObject(vaoPlane);
	WRender::DeleteVertexArrayObject(sqVao);

	WRender::DeleteBuffer(abColour);
	WRender::DeleteBuffer(abTorus);
	WRender::DeleteBuffer(abPlane);
	WRender::DeleteBuffer(sqAb);
	WRender::DeleteBuffer(sqEab);
	WRender::DeleteBuffer(ubo);
	
	WRender::DeleteFrameBuffer(fbo);	
	WRender::DeleteTexture(depthTexture);	
	glswShutdown();
}