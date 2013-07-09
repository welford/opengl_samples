#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <math.h>
#include "transforms.h"
#include "glsw.h"
#include "debug_print.h"
#include "nv_dds.h"

//PTT
static const float sqVertices[] = {
	//0
	 -1.0f, -1.0f, 0,
	 0,0,
	 //1
	 1.0f, -1.0f, 0,
	 1.0f,0,
	 //2
	 1.0f, 1.0f, 0,
	 1.0f,1.0f,
	 //3
	 -1.0f, 1.0f, 0,
	 0,1.0f
};

static unsigned char sqIndices[] = {0, 1, 3, 2 };

/*
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

//shaders
CShaderProgram program[3]; //0 normal,  1 downsample, 2 tonemapping

//buffer for drawing
unsigned int ubo=0;	
unsigned int sqVao=0, sqEab=0, sqAb=0;	

//FBO to render to
#define RENDER_WIDTH 1200
#define RENDER_HEIGHT 803
#define TEXTURE_WIDTH_HEIGHT 512

WRender::FrameBuffer::SObject main_fbo;			//Rendered at  RENDER_WIDTH*RENDER_HEIGHT
WRender::Texture::SObject texRGB;
WRender::Texture::SObject texD;

WRender::FrameBuffer::SObject downsampled_fbo;	//Rendered at  TEXTURE_WIDTH_HEIGHT*TEXTURE_WIDTH_HEIGHT
WRender::Texture::SObject texDownSample;		//stores downsampled luminance versions in mipnmap layers


WRender::Texture::SObject texRGBA16F;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);

int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"Hello", 4, 2, RENDER_WIDTH, RENDER_HEIGHT, 24, 8, 24, 8, CPlatform::MS_0);	
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


	glswInit();
	glswSetPath("../resources/", ".glsl");
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	
	//got = glswGetShadersAlt("Dbg.ScreenSpaceTexture.Vertex", pVertStr, 1);
	pVertStr[0] = glswGetShaders("shaders.Dbg.ScreenSpaceTexture.Vertex");
	pFragStr = glswGetShaders("shaders.Dbg.ScreenSpaceTexture.Fragment");
	CShader vertexShader(CShader::VERT, &pVertStr[0], 1);
	CShader fragmentShader(CShader::FRAG, &pFragStr, 1);


	pVertStr[0] = glswGetShaders("shaders.Post.Vertex");
	pFragStr = glswGetShaders("shaders.Post.HDR.Luminance.Fragment");
	CShader vertexShaderStage2(CShader::VERT, &pVertStr[0], 1);
	CShader fragmentShaderStage2(CShader::FRAG, &pFragStr, 1);

	pVertStr[0] = glswGetShaders("shaders.Post.Vertex");
	pFragStr = glswGetShaders("shaders.Post.HDR.VisibleRange.Fragment");
	CShader vertexShaderStage3(CShader::VERT, &pVertStr[0], 1);
	CShader fragmentShaderStage3(CShader::FRAG, &pFragStr, 1);

	// - - - - - - - - - - 
	//setup the shaders
	program[0].Initialise();
	program[0].AddShader(&vertexShader);
	program[0].AddShader(&fragmentShader);
	program[0].Link();

	program[1].Initialise();
	program[1].AddShader(&vertexShaderStage2);
	program[1].AddShader(&fragmentShaderStage2);
	program[1].Link();
	program[1].Start();
	program[1].SetTextureUnit("src",0);
	program[1].Stop();

	program[2].Initialise();
	program[2].AddShader(&vertexShaderStage3);
	program[2].AddShader(&fragmentShaderStage3);
	program[2].Link();
	program[2].Start();
	program[2].SetTextureUnit("src",0);
	program[2].SetTextureUnit("avg_lum",1);
	program[2].Stop();

	// - - - - - - - - - - 
	//setup the textures

	//source dds image
	WRender::Texture::SDescriptor descImage = {WRender::Texture::TEX_2D, WRender::Texture::RGBA16F, 512, 512, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};

	//1st pass
	WRender::Texture::SDescriptor descMain = {WRender::Texture::TEX_2D, WRender::Texture::RGBA16F, RENDER_WIDTH, RENDER_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Texture::SDescriptor descDepth = {WRender::Texture::TEX_2D, WRender::Texture::DEPTH_COMPONENT, RENDER_WIDTH, RENDER_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};

	//2nd pass
	WRender::Texture::SDescriptor descDownsample = {WRender::Texture::TEX_2D, WRender::Texture::RGBA16F, TEXTURE_WIDTH_HEIGHT, TEXTURE_WIDTH_HEIGHT, 0, 0, WRender::Texture::GEN_MIPMAP};
	

	WRender::Texture::SParam param[] ={	
		{ WRender::Texture::MIN_FILTER, WRender::Texture::LINEAR},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},		
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},

		{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST_MIPMAP_NEAREST},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},		
		{ WRender::Texture::WRAP_S, WRender::Texture::CLAMP_TO_EDGE},
		{ WRender::Texture::WRAP_T, WRender::Texture::CLAMP_TO_EDGE},
	};

	//for first pass
	WRender::CreateBaseTexture(texRGB, descMain);
	WRender::SetTextureParams(texRGB,param,4);

	WRender::CreateBaseTexture(texD, descDepth);
	WRender::SetTextureParams(texD,param,4);

	//for 2nd pass
	WRender::CreateBaseTexture(texDownSample, descDownsample);
	WRender::SetTextureParams(texDownSample,param+4,4);

	//for HDR image 
	nv_dds::CDDSImage image;
	image.load("../resources/Habib_House_Med.dds");
	descImage.w = image.get_width();
	descImage.h = image.get_height();

	WRender::Pixel::Data pixelData = {WRender::Pixel::RGBA, WRender::Pixel::HALF_FLOAT, 0, image};
	WRender::CreateBaseTextureData( texRGBA16F, descImage, pixelData );
	WRender::SetTextureParams(texRGBA16F,param,4);

	// - - - - - - - - - - 
	//setup Frame Buffer

	//first pass
	WRender::CreateFrameBuffer(main_fbo);
	WRender::AddTextureRenderBuffer(main_fbo, texRGB, WRender::ATT_CLR0, 0);
	WRender::AddTextureRenderBuffer(main_fbo, texD, WRender::ATT_DEPTH, 0);
	WRender::CheckFrameBuffer(main_fbo);

	//2nd pass
	WRender::CreateFrameBuffer(downsampled_fbo);
	WRender::AddTextureRenderBuffer(downsampled_fbo, texDownSample, WRender::ATT_CLR0, 0);
	WRender::CheckFrameBuffer(downsampled_fbo);

	//square
	sqVao = WRender::CreateVertexArrayObject();	
	sqEab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(sqIndices), sqIndices);
	sqAb = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(sqVertices), sqVertices);
	WRender::BindVertexArrayObject(sqVao);
	WRender::BindBuffer(WRender::ELEMENTS, sqEab);
	WRender::VertexAttribute sqVa[2] = {
		{sqAb, 0, 3, WRender::FLOAT, 0, sizeof(float)*5, 0, 0},				//vertices
		{sqAb, 1, 2, WRender::FLOAT, 0, sizeof(float)*5, sizeof(float)*3, 0},	//texture coordinates
	};
	WRender::SetAttributeFormat( sqVa, 2, 0);
	WRender::UnbindVertexArrayObject();

	//ubo for cameras etc
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
}

static float key = 0.18f;
static float smallest_pure_white = 1.0f;
static bool limited_range = false;

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	CVec3df cameraPosition(0, 0.0f, -2.1f);
	pPlatform->Tick();


	//- - - - - - - - - - - - - - - - - - - 
	//first pass to texture
	WRender::EnableDepthTest(false);
	{
		WRender::SetClearColour(1.0,0,0,0);
		WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, main_fbo);
		WRender::SetupViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
		WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);

		program[0].Start();
		WRender::BindTexture(texRGBA16F, WRender::Texture::UNIT_0);
		WRender::BindVertexArrayObject( sqVao );		
		WRender::Draw( WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0 );

		WRender::GenerateTextureMipMap(texDownSample); //generate mipmaps
	}

	//- - - - - - - - - - - - - - - - - - - 
	//2nd pass, create luminance texture and find log average luminance
	{
		WRender::SetClearColour(1.0,0,0,0);
		WRender::BindFrameBuffer(WRender::FrameBuffer::DRAW, downsampled_fbo);
		WRender::SetupViewport(0, 0, TEXTURE_WIDTH_HEIGHT, TEXTURE_WIDTH_HEIGHT);
		

		program[1].Start();
		WRender::BindTexture(texRGB,WRender::Texture::UNIT_0);
		WRender::BindVertexArrayObject(sqVao);		
		WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);

		WRender::GenerateTextureMipMap(texDownSample); //generate mipmaps
	}

	WRender::UnbindFrameBuffer(WRender::FrameBuffer::DRAW);
	WRender::SetDrawBuffer(WRender::DB_BACK);
	
	//- - - - - - - - - - - - - - - - - - - 
	//3rd pass, normalise scene
	//to back buffer
	{
		//texture to screen
		WRender::SetClearColour(1.0,0,0,0);
		WRender::SetupViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
				
		program[2].Start();
		program[2].SetFloat("smallest_pure_white", smallest_pure_white);
		program[2].SetFloat("key", key);
		program[2].SetBool("limited_range", limited_range);

		WRender::BindTexture(texRGB,WRender::Texture::UNIT_0);
		WRender::BindTexture(texDownSample,WRender::Texture::UNIT_1);
		WRender::BindVertexArrayObject(sqVao);		
		WRender::Draw(WRender::TRIANGLE_STRIP, WRender::U_BYTE, sizeof(sqIndices)/sizeof(unsigned char), 0);
	}
	
	if ( pPlatform->GetKeyboard().keys[KB_1].IsToggledPress() ){
		limited_range = false;
		d_printf("using equation 3 : map full range\n");
	}

	if ( pPlatform->GetKeyboard().keys[KB_2].IsToggledPress() ){
		limited_range = true;
		d_printf("using equation 3 : map limited range\n");
	}

	pPlatform->UpdateBuffers();
	if ( !pPlatform->GetKeyboard().keys[KB_LEFTSHIFT].IsPressed() ){
		if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed()){
			smallest_pure_white += 0.1f * pPlatform->GetDT();
			d_printf("smallest_pure_white:%f\n", smallest_pure_white);
		}
		if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed()){
			smallest_pure_white -= 0.1f * pPlatform->GetDT();
			if(smallest_pure_white < 0.0f)
				smallest_pure_white = 0.0f;
			d_printf("smallest_pure_white:%f\n", smallest_pure_white);
		}
	}
	else{
		if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed()){
			key += 0.1f * pPlatform->GetDT();
			d_printf("key:%f\n", key);
		}
		if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed()){
			key -= 0.1f * pPlatform->GetDT();
			if(key < 0.0f)
				key = 0.0f;
			d_printf("key:%f\n", key);
		}
	}
}

void CleanUp(void)
{	
	WRender::DeleteBuffer(ubo);
	
	WRender::DeleteVertexArrayObject(sqVao);
	WRender::DeleteBuffer(sqAb);
	WRender::DeleteBuffer(sqEab);	

	WRender::DeleteTexture(texRGB);
	WRender::DeleteTexture(texD);
	WRender::DeleteFrameBuffer(main_fbo);

	program[0].CleanUp();
	program[1].CleanUp();
	glswShutdown();
}