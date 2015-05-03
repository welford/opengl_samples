#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include "transforms.h"
#include "glsw.h"

#include <GL/glew.h>

/*
Generating vertices for some shapes
*/
typedef struct {
	CMatrix44 mvp;
	CMatrix44 proj; 
	CMatrix44 mv;
	float nrm[16]; //3x3 matrix	
} Transforms;

//triangle's vertices
static const float vertices[] = {
	//0
	-0.5f, -0.5f, 0.0f, //P
	 1.0f,  0.0f, 0.0f, //C
	 //1
	 0.5f, -0.5f, 0.0f, //P
	 0.0f,  0.0f, 1.0f, //C
	 //2
	 0.0f, +0.5f, 0.0f, //P
	 0.0f,  1.0f, 0.0f, //C
};


//static vars 
CPlatform* pPlatform = 0;
Transform transform(10);
float latitude = 0.0f, longitude = 0.0f;
Transforms transforms;

//buffer for drawing
unsigned int ubo =0;
//normal triangle stuff
unsigned int vao, ab;

//tranform feedback stuff
unsigned int tranform_feedback_buffer_pos = 0;
unsigned int tranform_feedback_buffer_clr = 0;
static int xbo, tf_vao, tf_ab;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"Transform Feedback Object. www.phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);	
	Setup(pPlatform);
	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	unsigned int got = 0;
	const char *pVertStr[3] = {0,0,0}, *pFragStr = 0;

	int nFacets = 0;

	glswInit();
	glswSetPath("../resources/", ".glsl");
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	
	got = glswGetShadersAlt("shaders.Version+shaders.Shared+shaders.TransformFeedbackObject", pVertStr, 3);
	pFragStr = glswGetShaders("shaders.Version+shaders.TransformFeedbackObjectFragment");
	CShader vertexShader(CShader::VERT, pVertStr, got);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);
	CShaderProgram program;

	const char * p_outputs[] = { "position", "gl_NextBuffer", "colour", "colour1", 0 };

	// - - - - - - - - - - 
	//setup the shaders
	program.Initialise();
	program.AddShader( &vertexShader );
	program.AddShader( &fragmentShader );	
	program.RecordOutput( p_outputs, CShaderProgram::INTERLEAVED );
	program.Link();

	// - - - - - - - - - - 
	//set up shapes	
	//Triangle
	vao = WRender::CreateVertexArrayObject();
	ab = WRender::CreateBuffer( WRender::ARRAY, WRender::DYNAMIC, sizeof(float) * 3 * 2 * 3, vertices );	
	WRender::VertexAttribute va[2] = {
		{ ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0,				0 },
		{ ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3,	0 },		
	};		
	WRender::BindVertexArrayObject( vao );
	WRender::SetAttributeFormat( va, 2, 0);
	WRender::BindVertexArrayObject( 0 );

	//transform feedback buffer	
	tranform_feedback_buffer_pos = WRender::CreateBuffer( WRender::FEEDBACK, WRender::DYNAMIC, sizeof(float) * 3 * 3, 0 );
	tranform_feedback_buffer_clr = WRender::CreateBuffer( WRender::FEEDBACK, WRender::DYNAMIC, sizeof(float) * 3 * 2 * 3, 0 );

	xbo = WRender::CreateTransformFeedback();
	WRender::BindTransformFeedback(xbo);
	WRender::BindBufferToIndex( WRender::FEEDBACK, tranform_feedback_buffer_pos, 0 ); //stores position
	WRender::BindBufferToIndex( WRender::FEEDBACK, tranform_feedback_buffer_clr, 1 ); //stores colour and colour1
	WRender::UnbindTransformFeedback();

	//Transform feedback
	
	tf_vao = WRender::CreateVertexArrayObject();
	WRender::VertexAttribute va_tf[2] = {
		{ tranform_feedback_buffer_pos, 0, 3, WRender::FLOAT, 0, sizeof(float)*3, 0,	0 },
		{ tranform_feedback_buffer_clr, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, 0,	0 },		
	};
	
	WRender::BindVertexArrayObject( tf_vao );
	WRender::SetAttributeFormat( va_tf, 2, 0);
	WRender::BindVertexArrayObject( 0 );	

	program.Start();
	WRender::BindTransformFeedback(xbo);
}

void MainLoop(CPlatform * const  pPlatform){	
	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::EnableDepthTest(true);
							
	// Start the transform feedback
	//	sends what we render to the tranform_feedback_buffer
	WRender::BeginTransformFeedback(WRender::TRIANGLES);

	WRender::BindVertexArrayObject(vao);	
	WRender::DrawArray( WRender::TRIANGLES, 3, 0 );
			
	WRender::EndTransformFeedback();

	float tmp[10];
	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(tmp), tmp);
	tmp[9] = 0;
	
	WRender::BindVertexArrayObject(tf_vao);	
	//WRender::DrawArray( WRender::TRIANGLES, 3, 0 );	
	WRender::DrawTransformFeedback( WRender::TRIANGLES, xbo );	
	
	pPlatform->UpdateBuffers();	
}

void CleanUp(void)
{	
	//WRender::DeleteVertexArrayObject(vaoSphere);
	WRender::DeleteBuffer( tranform_feedback_buffer_clr );
	WRender::DeleteBuffer( tranform_feedback_buffer_pos );
	WRender::DeleteBuffer( ab );
	WRender::DeleteVertexArrayObject( vao );
	WRender::DeleteVertexArrayObject( tf_vao );
	WRender::DeleteTransformFeedback( xbo );

	glswShutdown();
}