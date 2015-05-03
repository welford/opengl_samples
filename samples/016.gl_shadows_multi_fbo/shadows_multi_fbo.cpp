#include <platform.h>
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <shapes.h>
#include "transforms.h"
#include "glsw.h"

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

#define NUM_SHADOWS 8
#define xstr(s) str(s)                                                          
#define str(s) #s  
#define NUM_SHADOWS_CHAR xstr (NUM_SHADOWS) //same as above

CMatrix44 shadowMtx[NUM_SHADOWS];
#define TEX_DIMENSIONS 256

CPlatform* pPlatform = 0;
Transform transform(10);
Transform shadow_transform[NUM_SHADOWS]; //alloc to size of NUM_SHADOWS
Transforms transforms;

//Various OpenGL buffers
unsigned int ubo=0, shadowUBO = 0;
unsigned int abColour=0;
unsigned int vaoTorus=0, abTorus=0;
unsigned int vaoPlane=0, abPlane=0;
unsigned int sqVao=0, sqEab=0, sqAb=0;
WRender::FrameBuffer::SObject fbo[NUM_SHADOWS];
WRender::Texture::SObject depthTexture[NUM_SHADOWS];

//for the camera
float latitude = 45.0f, longitude = 45.0f;
float lightLatitude = 45.0f, lightLongitude = 0.0f;
float *pLat = &latitude, *pLong = &longitude;
CVec3df cameraPosition(0, 0.0f, -15.0f);
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
	pPlatform->Create(L"shadows example, phaseronkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);	
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
	const char *pVertStr[3] = {0,0,0}, *pFragStr = 0;
	const char *pFragTexStr = 0, *pVertTexStr = 0;

	//fbo stuff

	WRender::Texture::SDescriptor descDepth = {WRender::Texture::TEX_2D, WRender::Texture::DEPTH_COMPONENT24, TEX_DIMENSIONS, TEX_DIMENSIONS, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SParam param[] ={	
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::WRAP_S, WRender::Texture::CLAMP_TO_EDGE},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
	};	
	
	glswInit();
	glswSetPath("../resources/", ".glsl");
	glswAddDirectiveToken("MultiShadow", "#define NUM_SHADOWS "NUM_SHADOWS_CHAR);

	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);

	// - - - - - - - - - - 
	//setup the shaders
	// - - - - - - - - - - 
	//normal shader
	glswGetShadersAlt("shaders.Version+shaders.Shared+shaders.MultiShadow.Vertex", pVertStr, 3);
	pFragStr = glswGetShaders("shaders.Version+shaders.MultiShadow.Fragment");
	CShader vertexShader(CShader::VERT, pVertStr, 3);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);
	
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();
	uShadowMtx = program.GetUniformLocation("Shadows.shadowMtx");
	program.Start();
	program.SetTextureUnit("shadowMap", 0);
	program.Stop();

	//debug shader for textures in screen space
	pVertTexStr = glswGetShaders("shaders.Version+shaders.Dbg.ScreenSpaceTexture.Vertex");
	pFragTexStr = glswGetShaders("shaders.Version+shaders.Dbg.ScreenSpaceTexture.Fragment");
	CShader vTexShader(CShader::VERT, &pVertTexStr, 1);
	CShader fTexShader(CShader::FRAG, &pFragTexStr, 1);	

	textureShader.Initialise();
	textureShader.AddShader(&vTexShader);
	textureShader.AddShader(&fTexShader);
	textureShader.Link();
	textureShader.Start();
	textureShader.SetTextureUnit("texture",1);
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
		{sqAb, 0, 3, WRender::FLOAT, 0, sizeof(float)*5, 0, 0},					//vertices
		{sqAb, 1, 2, WRender::FLOAT, 0, sizeof(float)*5, sizeof(float)*3, 0},	//texture coordinates
	};
	WRender::SetAttributeFormat( sqVa, 2, 0);
	WRender::UnbindVertexArrayObject();

	// - - - - - - - - - - 
	//ubo for cameras etc
	// - - - - - - - - - - 
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	shadowUBO = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(CMatrix44)*(NUM_SHADOWS+1), shadowMtx);
	WRender::BindBufferToIndex(WRender::UNIFORM, shadowUBO, 2);

	// - - - - - - - - - - 
	//FBO for shadows
	// - - - - - - - - - - 
	//create the texture and FBO for rendering to when drawing 
	//during shadow stage
	for(unsigned int i=0;i<NUM_SHADOWS; i++)
	{
		WRender::CreateFrameBuffer(fbo[i]);	
		WRender::ActiveTexture(WRender::Texture::Unit(WRender::Texture::UNIT_0+i));
		WRender::CreateBaseTexture(depthTexture[i], descDepth);	
		WRender::SetTextureParams(depthTexture[i],param,4);

		WRender::AddTextureRenderBuffer(fbo[i], depthTexture[i], WRender::ATT_DEPTH, 0);					//add it to the FBO
		WRender::BindTexture(depthTexture[i],WRender::Texture::Unit(WRender::Texture::UNIT_0+i));	//bind it to a tex unit		
		WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo[i]);
		WRender::SetDrawBuffer(WRender::DB_NONE);
		WRender::CheckFrameBuffer(fbo[i]);
	}		

	//set the projection matrix
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);	

	for(unsigned int i=0;i<NUM_SHADOWS; i++)
		shadow_transform[i].ResizeStack(10);
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
	for(unsigned int i=0;i<NUM_SHADOWS; i++)
	{
		transform.Push();			
		{
			WRender::SetupViewport(0, 0, TEX_DIMENSIONS, TEX_DIMENSIONS);	//adjust viewport
			WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, fbo[i]);		//set fbo 
			WRender::SetDrawBuffer(WRender::DB_NONE);						//don't need any draw buffers	
			WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);				//clear draw buffers (must be called after we set the fbo)

			rotLat.Identity();	rotLat.Rotate(lightLatitude, 1, 0, 0);
			rotLong.Identity();	rotLong.Rotate(lightLongitude+(i*(360.0f/float(NUM_SHADOWS))), 0, 1, 0);

			transform.Translate(cameraPosition);			
			transform.ApplyTransform(rotLat);
			transform.ApplyTransform(rotLong);

			//setup shadow mvp			
			shadow_transform[i].Identity();
			shadow_transform[i].Translate(0.5f, 0.5f, 0.5f);
			shadow_transform[i].Scale(0.5f,0.5f,0.5f);
			shadow_transform[i].ApplyTransform(transforms.proj);
			shadow_transform[i].ApplyTransform(transform.GetCurrentMatrix());

			DrawGeometry(true);
			WRender::UnbindFrameBuffer(WRender::FrameBuffer::DRAW); //reset draw buffer
			WRender::SetupViewport(0, 0, 640, 640);	//reset viewport			
		}
		transform.Pop();		
	}

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
	WRender::BindTexture(depthTexture[0],WRender::Texture::Unit(WRender::Texture::UNIT_0));
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

#define ANIM_TORUS

void DrawGeometry(bool shadowPass)
{
	CMatrix44 rot;
	rot.Rotate(45,1,0,0);
#ifdef ANIM_TORUS
	static float angle = 0;
	CMatrix44 anim;
	anim.Rotate(angle,0,1,0);
	angle += 0.1f;
#endif

	transform.Push();
	if(shadowPass == false)
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadow_transform[i].Push();

	transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
	WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
	if(shadowPass == false)
	{
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadowMtx[i] = shadow_transform[i].GetCurrentMatrix();
		WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, shadowUBO, sizeof(CMatrix44)*NUM_SHADOWS, (void*)shadowMtx, 0);
		//WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, shadowUBO, sizeof(CMatrix44)*NUM_SHADOWS, (void*)shadowMtx, sizeof(CMatrix44));
	}
	//Plane
	WRender::BindVertexArrayObject(vaoPlane);
	WRender::DrawArray(WRender::TRIANGLES, 4*3, 0);

	transform.Pop();
	if(shadowPass == false)
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadow_transform[i].Pop();
	
	transform.Push();
	if(shadowPass == false)
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadow_transform[i].Push();
	
	#ifdef ANIM_TORUS
	transform.ApplyTransform(anim);
	#endif
	transform.Translate(0, 3.0f, 5.0f);
	transform.ApplyTransform(rot);	

	if(shadowPass == false)
	{
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
		{
			#ifdef ANIM_TORUS
			shadow_transform[i].ApplyTransform(anim);
			#endif
			shadow_transform[i].Translate(0, 3.0f, 5.0f);
			shadow_transform[i].ApplyTransform(rot);			
		}
	}
	transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
	WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
	if(shadowPass == false)
	{
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadowMtx[i] = shadow_transform[i].GetCurrentMatrix();
		WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, shadowUBO, sizeof(CMatrix44)*NUM_SHADOWS, (void*)shadowMtx, sizeof(CMatrix44));
	}
	//Torus
	WRender::BindVertexArrayObject(vaoTorus);
	WRender::DrawArray(WRender::TRIANGLES, nTorusVertices, 0);

	transform.Pop();
	if(shadowPass == false)
	{
		for(unsigned int i=0;i<NUM_SHADOWS; i++)
			shadow_transform[i].Pop();
	}
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
	WRender::DeleteBuffer(shadowUBO);

	for(unsigned int i=0;i<NUM_SHADOWS; i++)
	{
		WRender::DeleteFrameBuffer(fbo[i]);
		WRender::DeleteTexture(depthTexture[i]);	
	}
	glswShutdown();
}