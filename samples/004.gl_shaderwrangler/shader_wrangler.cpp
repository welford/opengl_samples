#include "platform.h"
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"
#include "transforms.h"
#include "glsw.h"
#include <gl/glew.h>

/*
simple example showing sharing values across two shaders
*/

static const float vertices[] = {
	//0
	 0,0,0, //P
	 1.0f,0.3f,0.3f, //C
	 //1
	 1.0f,0,0, //P
	 1.0f,0.3f,0.3f, //C
	 //2
	 0,0,0, //P
	 0.3f,1.0f,0.3f, //C
	 //3
	 0,1.0f,0, //P
	 0.3f,1.0f,0.3f, //C
	 //4
	 0,0,0, //P
	 0.3f,0.3f,1.0f, //C
	 //5
	 0,0,1.0f, //P
	 0.3f,0.3f,1.0f, //C
};
static unsigned char indices[] = {	0, 1, 2, 3, 4, 5 };


typedef struct {
	CMatrix44 mvp;
	CMatrix44 proj; 
	CMatrix44 mv;
	float nrm[16]; //3x3 matrix	
} Transforms;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//static variables 
CPlatform* pPlatform = GetPlatform();	
Transform transform(10);
Transforms transforms;	
CShaderProgram program[2];
unsigned int vao=0, eab=0, ab=0, ubo=0;	
float latitude = 45.0f, longitude = 45.0f;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);


int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"shader wrangler example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	{
		glswInit();
		glswSetPath("./", ".vertfrag");
	}

	{
	const char* pStr[2] = {0,0};
	int got = glswGetShadersAlt("shaders.Shared+shaders.Vertex", pStr, 2);
	const char* pV2 = glswGetShaders("shaders.Shared+shaders.Vertex2");
	const char* pF = glswGetShaders("shaders.Fragment");

	CShader vertexShader(CShader::VERT, pStr, got);
	CShader vertexShader2(CShader::VERT, &pV2, 1);
	CShader fragmentShader(CShader::FRAG, &pF, 1);	
	
	// - - - - - - - - - - 
	//setup the shaders	

	program[0].Initialise();
	program[0].AddShader(&vertexShader);
	program[0].AddShader(&fragmentShader);
	program[0].Link();

	program[1].Initialise();
	program[1].AddShader(&vertexShader2);
	program[1].AddShader(&fragmentShader);
	program[1].Link();

	/*
	int blockAlign = CShaderProgram::GetUniformBlockAlignment();
	int blockLoc = program[0].GetUniformBlockBinding(program[0].GetUniformBlockLocation("Transforms")); //should be 1 :)
	int blockSiz = program[0].GetUniformBlockSize(program[0].GetUniformBlockLocation("Transforms")); //should be 1 :)
	int namelength = program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::NAME_LENGTH);
	int activeUniforms = program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::BLK_ACTIVE_UNIFORMS);
	int uniformindices[100];	
	program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::BLK_ACTIVE_UNIFORMS_INDICES, uniformindices);
	*/

	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[2] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},		
	};
	WRender::SetAttributeFormat( va, 2, 0);	
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);	
	}
}

void MainLoop(CPlatform * const  pPlatform)
{	
	CVec3df cameraPosition(0, 0.0f, -3.0f);
	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 		
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
			//program.SetMtx44(uMVP, transforms.mvp.data);
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
				
			program[0].Start();
			WRender::Draw(WRender::LINES, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			WRender::Draw(WRender::POINTS, WRender::U_BYTE, 1, 5);

			program[1].Start();				
			WRender::Draw(WRender::LINES, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			WRender::Draw(WRender::POINTS, WRender::U_BYTE, 1, 5);
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
	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);
	WRender::DeleteBuffer(ubo);
	//shaders were sorted when they go out of scope
	program[0].CleanUp();
	program[1].CleanUp();	
	glswShutdown();
	//GLenum a = glGetError();
}