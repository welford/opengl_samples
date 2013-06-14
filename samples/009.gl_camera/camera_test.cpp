#include "platform.h"
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"
#include "transforms.h"


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


static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
uniform mat4 mvp;\n\
uniform mat4 mv;\n\
out vec3 colour;\n\
void main()\n\
{\n\
	colour = inColour;\n\
	vec4 finalPosition = mvp * vec4(inVertex,1) ;\n\
	gl_Position = finalPosition;\n\
	gl_PointSize = 10.0f;\n\
	//gl_Position = vec4(inVertex,1);\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
out vec4 fragColour;\n\
void main()\n\
{\n\
float d = (1.0 - gl_FragCoord.z);\n\
fragColour = vec4(colour.xyz*d, 1);\n\
}\n\
";

//static variables 
CPlatform* pPlatform = 0;
Transform transform(10);
unsigned int vao=0, eab=0, ab=0;	
int uMVP=0, uMV = 0;
CShaderProgram program;
float latitude = 0.0f, longitude = 0.0f;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"camera example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{	
	CShader vertexShader(CShader::VERT, &pVertexShader, 1);
	CShader fragmentShader(CShader::FRAG, &pFragmentShader, 1);

	// - - - - - - - - - - 
	//setup the shaders	
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();
	uMVP = program.GetUniformLocation("mvp");
	uMV = program.GetUniformLocation("mv");
	
	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[2] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},		
	};
	WRender::SetAttributeFormat( va, 2, 0);		
	program.Start();
	WRender::EnableCulling(true);
	
}

void MainLoop(CPlatform * const  pPlatform)
{	
	CMatrix44 projection,camera_rotation, mvp;
	Transform::CreateProjectionMatrix(projection, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
	//update the main application
	CVec3df cameraPosition(0, 0.0f, -2.0f);
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
			mvp = projection * transform.GetCurrentMatrix();
			program.SetMtx44(uMVP, mvp.data);
			program.SetMtx44(uMV, transform.GetCurrentMatrix().data);
			//program.SetMtx44(uMVP,transform.GetCurrentMatrix().data);			
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
{	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);	
	program.CleanUp();
}