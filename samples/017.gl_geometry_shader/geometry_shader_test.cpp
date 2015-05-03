#include <platform.h>
#include <wrapper_gl.hpp>
#include <shader_gl.hpp>
#include <glsw.h>
#include <gl/glew.h>

/*
simple example showing sharing values across two shaders
*/
static const float vertices[] = {
	//0
	-0.5f, -0.5f, 0.0f, //P
	 //1
	 +0.5f, -0.5f, 0.0f, //P
	 //2
	 0.0f, +0.5f, 0.0f, //P
};

//static variables 
CPlatform* pPlatform = GetPlatform();	
CShaderProgram program;
unsigned int vao=0, ab=0;	

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);


int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"geometry shader example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
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
		glswSetPath("../resources/", ".glsl");
	}

	const char* pV = glswGetShaders("shaders.Version+shaders.PassThroughVertex");
	//pass though shader
	//const char* pG = glswGetShaders("shaders.Version+shaders.PassThroughGeometry");
	//const char* pF = glswGetShaders("shaders.Version+shaders.PassThroughFragment");
	//add extra attributes at geometry stage
	const char* pG = glswGetShaders("shaders.Version+shaders.AddColorGeometry");
	const char* pF = glswGetShaders("shaders.Version+shaders.AddColorFragment");
	//duplicates geometry
	//const char* pG = glswGetShaders("shaders.Version+shaders.DuplicatePrimitiveGeometry");
	//const char* pF = glswGetShaders("shaders.Version+shaders.PassThroughFragment");

	CShader vertexShader(CShader::VERT, &pV, 1);
	CShader geometryShader(CShader::GEOM, &pG, 1);
	CShader fragmentShader(CShader::FRAG, &pF, 1);
	// - - - - - - - - - - 
	//setup the shaders	

	program.Initialise();
	program.AddShader(&vertexShader);
	program.AddShader(&geometryShader);
	program.AddShader(&fragmentShader);
	program.Link();

	// - - - - - - - - - - 
	// setup vertex buffers etc
	vao = WRender::CreateVertexArrayObject();
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	
	WRender::BindVertexArrayObject(vao);
	WRender::VertexAttribute va[2] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0, 0},
	};
	WRender::SetAttributeFormat( va, 1, 0);	

	program.Start();
	WRender::SetClearColour(0,0,0,0);
}

void MainLoop(CPlatform * const  pPlatform)
{	
	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 			
	WRender::DrawArray(WRender::TRIANGLES, 3, 0);
	pPlatform->UpdateBuffers();
}

void CleanUp(void)
{	
	//shaders were sorted when they went out of scope
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	//shaders were sorted when they go out of scope
	program.CleanUp();
	glswShutdown();
}