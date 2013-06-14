#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <shapes.h>
#include "transforms.h"
#include "glsw.h"

/*
Generating vertices for some shapes
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

//buffer for drawing
unsigned int ubo=0, abColour=0;	
unsigned int vaoSphere=0, abSphere=0;	
unsigned int vaoCylinder=0, abCylinder=0;	
unsigned int vaoCone=0, abCone=0;	
unsigned int vaoTorus=0, abTorus=0;	
unsigned int vaoFrustum=0, abFrustum=0;	

//amount of vertices to draw
unsigned int nSphereVertices;
unsigned int nCylinderVertices;
unsigned int nConeVertices;	
unsigned int nTorusVertices;

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
	const char *pVertStr[2] = {0,0}, *pFragStr = 0;

	unsigned int nSphereFloats;
	float *pSphereVertices = 0;
	unsigned int iterations = 3;

	unsigned int nCylinderFloats;
	float *pCylinderVertices = 0;
	unsigned int segments = 20;

	unsigned int nConeFloats;
	float *pConeVertices = 0;
	unsigned int coneSegments = 20;

	unsigned int nTorusFloats;
	float *pTorusVertices = 0;
	unsigned int torusSegments = 20;
	unsigned int torusTubeSegments = 10;

	float *pFrustumVertices = 0;

	float colour[] = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f};
	int nFacets = 0;

	glswInit();
	glswSetPath("../resources/", ".glsl");
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	
	got = glswGetShadersAlt("shaders.Shared+shaders.BasicVertexShader", pVertStr, 2);
	pFragStr = glswGetShaders("shaders.BasicFragmentShader");
	CShader vertexShader(CShader::VERT, pVertStr, got);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);
	CShaderProgram program;

	// - - - - - - - - - - 
	//setup the shaders
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();

	// - - - - - - - - - - 
	//set up shapes
	//Shared
	abColour = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(colour), colour);

	//SPHERE
	nSphereVertices = unsigned int(powl(4.0,long double(iterations)))*8*3;
	nSphereFloats = nSphereVertices*3;
	pSphereVertices = new float[nSphereFloats];
	nFacets = CreateNSphere(pSphereVertices, iterations);
	vaoSphere = WRender::CreateVertexArrayObject();
	abSphere = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nSphereFloats, pSphereVertices);	

	WRender::BindVertexArrayObject(vaoSphere);
	WRender::VertexAttribute vaSphere[2] = {
		{abSphere, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 1},
	};
	WRender::SetAttributeFormat( vaSphere, 2, 0);
	delete[] pSphereVertices;

	//CYLINDER
	nCylinderVertices = segments*4*3;
	nCylinderFloats = nCylinderVertices*3;
	pCylinderVertices = new float[nCylinderFloats];
	nFacets = CreateCylinder(pCylinderVertices, segments, 1.0f, 1.0f);	

	vaoCylinder = WRender::CreateVertexArrayObject();
	abCylinder = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nCylinderFloats, pCylinderVertices);	

	WRender::BindVertexArrayObject(vaoCylinder);
	WRender::VertexAttribute vaCylinder[2] = {
		{abCylinder, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 1},
	};
	WRender::SetAttributeFormat( vaCylinder, 2, 0);
	delete[] pCylinderVertices;

	//CONE
	nConeVertices = coneSegments*4*3;
	nConeFloats = nConeVertices*3;
	pConeVertices = new float[nConeFloats];
	nFacets = CreateCone(pConeVertices, coneSegments, 5.0f, 1.0f);

	vaoCone = WRender::CreateVertexArrayObject();
	abCone = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nConeFloats, pConeVertices);	

	WRender::BindVertexArrayObject(vaoCone);
	WRender::VertexAttribute vaCone[2] = {
		{abCone, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*3, sizeof(float)*3, 1},
	};
	WRender::SetAttributeFormat( vaCone, 2, 0);
	delete[] pConeVertices;

	//Torus
	nTorusVertices = 2*torusTubeSegments*torusSegments*3;
	nTorusFloats = nTorusVertices*3;
	pTorusVertices = new float[nTorusFloats];
	nFacets = CreateTorus(pTorusVertices, torusSegments, 5.0f, torusTubeSegments, 1.0f);	
	vaoTorus = WRender::CreateVertexArrayObject();
	abTorus = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*nTorusFloats, pTorusVertices);	

	WRender::BindVertexArrayObject(vaoTorus);
	WRender::VertexAttribute vaTorus[2] = {
		{abTorus, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*3, sizeof(float)*6, 1},
	};
	WRender::SetAttributeFormat( vaTorus, 2, 0);
	delete[] pTorusVertices;

	//Frustum
	pFrustumVertices = new float[12*3*3];
	nFacets = CreateFrustum(pFrustumVertices, -0.5f, 0.5f, -0.5f, 0.5f, 0.1f, 5.0f);
	vaoFrustum = WRender::CreateVertexArrayObject();
	abFrustum = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(float)*12*3*3, pFrustumVertices);	

	WRender::BindVertexArrayObject(vaoFrustum);
	WRender::VertexAttribute vaFrustum[2] = {
		{abFrustum, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
		{abColour, 1, 3, WRender::FLOAT, 0, sizeof(float)*3, sizeof(float)*3, 1},
	};
	WRender::SetAttributeFormat( vaFrustum, 2, 0);
	delete[] pFrustumVertices;

	//ubo for cameras etc
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
	program.Start();
}

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	CVec3df cameraPosition(0, 0.0f, -10.0f);

	pPlatform->Tick();	
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

		transform.Push();
		{
			transform.Translate(2.1f, 0, 0);
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			//Cylinder
			WRender::BindVertexArrayObject(vaoCylinder);
			WRender::DrawArray(WRender::TRIANGLES, nCylinderVertices, 0);				
		}
		transform.Pop();

		transform.Push();
		{
			transform.Translate(-2.1f, 0, 0);
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			//Cone
			WRender::BindVertexArrayObject(vaoCone);
			WRender::DrawArray(WRender::TRIANGLES, nConeVertices, 0);
		}
		transform.Pop();
			
		transform.Push();
		{
			CMatrix44 rot;
			rot.Rotate(45,1,0,0);
			transform.ApplyTransform(rot);
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			//Torus
			WRender::BindVertexArrayObject(vaoTorus);
			WRender::DrawArray(WRender::TRIANGLES, nTorusVertices, 0);
		}
		transform.Pop();
			
		transform.Push();
		{
			transform.Translate(0, 2.0f, 0);
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			//Cone
			WRender::BindVertexArrayObject(vaoFrustum);
			WRender::DrawArray(WRender::TRIANGLES, 12*3, 0);
		}
		transform.Pop();
	}
	transform.Pop();		
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
	WRender::DeleteVertexArrayObject(vaoTorus);
	WRender::DeleteVertexArrayObject(vaoSphere);
	WRender::DeleteVertexArrayObject(vaoCylinder);
	WRender::DeleteVertexArrayObject(vaoCone);
	WRender::DeleteVertexArrayObject(vaoTorus);
	WRender::DeleteVertexArrayObject(vaoFrustum);

	WRender::DeleteBuffer(ubo);
	WRender::DeleteBuffer(abColour);
	WRender::DeleteBuffer(abSphere);
	WRender::DeleteBuffer(abCylinder);
	WRender::DeleteBuffer(abCone);
	WRender::DeleteBuffer(abTorus);
	WRender::DeleteBuffer(abFrustum);

	glswShutdown();
}