#include "platform.h"
#include <GL/glew.h>
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"

/*
using the simple wrapper for openGL
*/
//PCCCNNN
static const float vertices[] = {
	//0
	-0.5f, -0.5f, 0.0f, //P
	 1.0f,  0.0f, 0.0f, //C
	 //1
	 +0.5f, -0.5f, 0.0f, //P
	 0.0f,  0.0f, 1.0f, //C

	 //2
	 0.0f, +0.5f, 0.0f, //P
	 0.0f,  1.0f, 0.0f, //C
};

static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
out vec3 colour;\n\
void main()\n\
{\n\
    colour = inColour;\n\
	gl_Position = vec4(inVertex.xyz,1.0);\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
out vec4 fragColour;\n\
void main()\n\
{\n\
    fragColour = vec4(colour, 1);\n\
	//fragColour = vec4(1,1,1, 1);\n\
}\n\
";
//static variables 
CPlatform* pPlatform = 0;
GLuint vao=0, ab=0;	

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);


int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"opengl wrapper example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
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
	CShaderProgram program;	

	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();

	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::VertexAttribute va[3] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},
	};
	WRender::SetAttributeFormat( va, 2, 0);
	program.Start();	
}

void MainLoop(CPlatform * const  pPlatform)
{	
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 
	WRender::DrawArray(WRender::TRIANGLES, 3, 0);		
	//swap buffers
	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{	
	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
}