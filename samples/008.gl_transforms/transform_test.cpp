#include "platform.h"
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"
#include "transforms.h"

#define SQUARE_HW 0.05f
//FMT
//PCCCNNN
//FMT
//PCCCNNN
static const float vertices[] = {
	//0
	 -SQUARE_HW,  -SQUARE_HW, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //1
	 SQUARE_HW,  -SQUARE_HW, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //2
	 SQUARE_HW,  SQUARE_HW, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //3
	 -SQUARE_HW,  SQUARE_HW, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
};
static unsigned char indices[] = {	0, 1, 3, 2 };

static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
uniform mat4 mt;\n\
out vec3 colour;\n\
void main()\n\
{\n\
    colour = inColour;\n\
	vec4 finalPosition = mt * vec4(inVertex,1);\n\
	gl_Position = finalPosition;\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
out vec4 fragColour;\n\
void main()\n\
{\n\
fragColour = vec4(colour.xyz, 1);\n\
}\n\
";

//static variables 
CPlatform* pPlatform = 0;
unsigned int vao=0, ab=0, eab=0;	
CShaderProgram program;
int mt = 0;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);


int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"transform example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
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
	mt = program.GetUniformLocation("mt");
	
	// - - - - - - - - - - 
	// setup vertex buffers etc
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[3] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},
	};
	WRender::SetAttributeFormat( va, 2, 0);
	program.Start();
}

void MainLoop(CPlatform * const  pPlatform)
{
	static float degrees[] = {0,0};
	CMatrix44 rot;	
	Transform transform(10);	

	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 
	transform.Push();
		program.SetMtx44(mt,transform.GetCurrentMatrix().data);
		WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);

		transform.Translate(0.2f,0.0,0.0);
		program.SetMtx44(mt,transform.GetCurrentMatrix().data);
		WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			

		transform.Push();				
			transform.Translate(-0.4f,0.0,0.0);
			rot.Identity();
			rot.Rotate(degrees[0],0,0,1);
			transform.ApplyTransform(rot);
			program.SetMtx44(mt,transform.GetCurrentMatrix().data);
			WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);

			transform.Push();
				transform.Translate(0.0,0.3f,0);
				program.SetMtx44(mt,transform.GetCurrentMatrix().data);
				WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			transform.Pop();
		transform.Pop();

		transform.Push();
			rot.Identity();
			rot.Rotate(degrees[1],0,0,1);
			transform.ApplyTransform(rot);
			transform.Translate(0.0,-0.3f,0.0);
			transform.ApplyTransform(rot);
			program.SetMtx44(mt,transform.GetCurrentMatrix().data);
			WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
		transform.Pop();
	transform.Pop();
	//swap buffers
	pPlatform->UpdateBuffers();

	degrees[0] -= 360.0f*pPlatform->GetDT();
	degrees[1] += 180.0f*pPlatform->GetDT();
	if(degrees[0] < 0.0f)
		degrees[0] += 360.0f;
	if(degrees[1] > 360.0f)
		degrees[1] -= 360.0f;
}

void CleanUp(void)
{	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);
}