#include "platform.h"
#include "wrapper_gl.hpp"
#include "shader_gl.hpp"


/*
showing clipspace
clip space is a box in which all that is drawn is contained
it's size is orthogonal and spans min(-1,-1,-1) to max (1,1,1)

z = -1 is closest to the screen and 
z = 1 is farthest from the screen

x = -1 is the laft hand side of the screen 
x = 1 is the right hand side of the screen

y = -1 is the bottom od the screen
y = 1 is the top of the screen
*/


#define SQUARES 10.0f
#define SQUARE_WH 1.0f
#define STEP_RANGE (2.0f-SQUARE_WH) //0 -> 2-SIZE
//FMT
//PCCCNNN
//FMT
//PCCCNNN
static const float vertices[] = {
	//0
	 0.0f,  0.0f, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //1
	 SQUARE_WH,  0.0f, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //2
	 SQUARE_WH,  SQUARE_WH, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
	 //3
	 0.0f,  SQUARE_WH, 0.0f, //P
	 1.0f,  1.0f, 1.0f, //C
};

static unsigned char indices[] = {	0, 1, 3, 2 };

static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
uniform vec3 offset;\n\
out vec3 colour;\n\
noperspective out vec3 clipspace;\n\
void main()\n\
{\n\
    colour = inColour;\n\
	gl_Position = vec4(inVertex.xyz + offset,1.0);\n\
	clipspace.xyz = inVertex.xyz + offset;\n\
}\n\
";

/*
in the fragment schader 

gl_FragCoord.z ranges between 0(nearest) and 1 (farthest) 
which is somewhat confusing as it's specified between 
-1.0 and 1.0 when passed from the vertex shader

gl_FragCoord.xy ranges between 0 and the screen resolution.xy

see http://www.phasersonkill.com/?p=495 for some other ideas
on how to get the coordinates mapped back into [-1,-1,-1]
to [1,1,1] and using the nonperspective slipspace stuff
*/
static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
noperspective in vec3 clipspace;\n\
out vec4 fragColour;\n\
void main()\n\
{\n\
fragColour = vec4(gl_FragCoord.x/640, 0, gl_FragCoord.z, 1);\n\
//fragColour = vec4(0.5f + (clipspace.x*0.5f), 0.5f + (clipspace.y*0.5f), 0.5f + (clipspace.z*0.5), 1);\n\
//fragColour = vec4(0, 0, 0.5f + (clipspace.z*0.5), 1);\n\
}\n\
";

//static variables 
CPlatform* pPlatform = 0;
unsigned int vao=0, eab=0, ab=0;	
CShaderProgram program;
int iOffset = 0;

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

	iOffset = program.GetUniformLocation("offset");
	
	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[3] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},
	};
	WRender::SetAttributeFormat( va, 3, 0);
	WRender::EnableDepthTest(true);
	program.Start();
}

void MainLoop(CPlatform * const  pPlatform)
{	
	float vOffset[3] = {-1.0f,-1.0f,-1.0f}; //change values

	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 
	vOffset[0] = vOffset[1] = vOffset[2] = -1.0f;
	for(unsigned int i=0;i<SQUARES;i++)
	{
		program.SetVec3(iOffset, vOffset);
		WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);		
		vOffset[0] += STEP_RANGE/(SQUARES-1);
		vOffset[1] += STEP_RANGE/(SQUARES-1);
		vOffset[2] += 2.0f/(SQUARES);
	}
	//swap buffers
	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);	
	program.CleanUp();
}