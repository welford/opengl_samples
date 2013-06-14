#include "platform.h"
#include <GL/glew.h>
#include "shader_gl.hpp"

/*
simple testbed for shaders
*/

//FMT
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

static unsigned char indices[] = {	0, 1, 2 };

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

#define BUFFER_OFFSET(i) ((char *)0 + (i))
GLuint vao=0, eab=0, ab=0;	
//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void DrawGeometry(bool shadowPass);
void CleanUp(void);

int PlatformMain( void ){
	CPlatform* pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"simple example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);	
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

	//setup the shaders
	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&fragmentShader);
	program.Link();

	glGenVertexArrays(1,&vao);	//vertex array object
	glGenBuffers(1, &eab);		//element array buffer
	glGenBuffers(1, &ab);		//array buffer

	//fill buffers
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, ab);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//bind vao and ste it's data
	glBindVertexArray(vao);
	//glBindBuffer(GL_ARRAY_BUFFER, ab);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, BUFFER_OFFSET(0)); //P
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, BUFFER_OFFSET(sizeof(float)*3)); //C
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);	
	glClearColor(0,0,0,0);
	program.Start();	

}

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	pPlatform->Tick();	
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements( GL_TRIANGLES, sizeof(indices)/sizeof(unsigned char), GL_UNSIGNED_BYTE, 0);	
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//swap buffers
	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{	//shaders were sorted when they went out of scope
	glDeleteVertexArrays(1, &vao);	
	glDeleteBuffers(1,&ab);
	glDeleteBuffers(1,&eab);
}