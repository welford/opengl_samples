#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include <shapes.h>
#include <shapes_util.h>
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
Transform transform_test(10);
float latitude = 0.0f, longitude = 0.0f, distance = -3.0f;
Transforms transforms;

//buffer for drawing
unsigned int ubo=0;	
//amount of vertices to draw
Shape sphere,cube;
Shape box;
CShaderProgram program_sphere;
CShaderProgram program_cube;

#define WIDTH_HEIGHT 16
WRender::Texture::SObject cubemap_tex;
WRender::Texture::Type cube_map_type[6] = {
	WRender::Texture::TEX_CUBE_MAP_POSITIVE_X,
	WRender::Texture::TEX_CUBE_MAP_POSITIVE_Y,
	WRender::Texture::TEX_CUBE_MAP_POSITIVE_Z,
	WRender::Texture::TEX_CUBE_MAP_NEGATIVE_X,
	WRender::Texture::TEX_CUBE_MAP_NEGATIVE_Y,
	WRender::Texture::TEX_CUBE_MAP_NEGATIVE_Z
};

unsigned char cubemap_clr[6][3] = {
	{255,	0,		0},
	{0,		255,	0},
	{0,		0,		255},
	{255,	255,	0},
	{0,		255,	255},
	{255,	0,		255},
};

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

void CreateCubeMapTextures(){

	WRender::Texture::SDescriptor desc = {WRender::Texture::TEX_CUBE_MAP, WRender::Texture::RGB8, WIDTH_HEIGHT, WIDTH_HEIGHT, 0, 0, WRender::Texture::DONT_GEN_MIPMAP};
	WRender::Pixel::Data pixelData = {WRender::Pixel::RGB, WRender::Pixel::UCHAR, 0, 0};
	WRender::Texture::SParam param[] ={
		{ WRender::Texture::MIN_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::MAG_FILTER, WRender::Texture::NEAREST},
		{ WRender::Texture::WRAP_S, WRender::Texture::REPEAT},
		{ WRender::Texture::WRAP_T, WRender::Texture::REPEAT},		
	};
	unsigned char * pTexture = new unsigned char[WIDTH_HEIGHT * WIDTH_HEIGHT * 3];

	WRender::ActiveTexture(WRender::Texture::UNIT_0);

	WRender::CreateBaseTexture(cubemap_tex, desc);
	for(unsigned int i=0;i<6;i++){
		for(unsigned int h=0;h<WIDTH_HEIGHT;h++)
		{
			for(unsigned int w=0;w<WIDTH_HEIGHT;w++)
			{
				float height_scale = float(h)/float(WIDTH_HEIGHT);
				pTexture[(h*WIDTH_HEIGHT*3)+(w*3)+0] = cubemap_clr[i][0]*height_scale;//*w_mod;	//R
				pTexture[(h*WIDTH_HEIGHT*3)+(w*3)+1] = cubemap_clr[i][1]*height_scale;//*w_mod;	//G
				pTexture[(h*WIDTH_HEIGHT*3)+(w*3)+2] = cubemap_clr[i][2]*height_scale;//*w_mod;	//B
			}	
		}

		pixelData.pData = (void*)pTexture;
		desc.type = cube_map_type[i];
		WRender::CreateTextureData( cubemap_tex, desc, pixelData );	
	}
	WRender::SetTextureParams( cubemap_tex, param, 4 );
	delete [] pTexture;	
}

void Setup(CPlatform * const  pPlatform)
{
	unsigned int got = 0;
	const char *pVertStr[2] = {0,0}, *pFragStr[2] = {0,0};
	float *pFrustumVertices = 0;
	float colour[] = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f};
	int nFacets = 0;

	glswInit();
	glswSetPath( "../resources/", ".glsl" );
	WRender::SetClearColour(0,0,0,0);	
	WRender::EnableCulling(true);
	
	// - - - - - - - - - - - - - - - - - - - -
	// - - - - - - - - - - - - - - - - - - - -
	glswGetShadersAlt( "shaders.Shared+shaders.Skymap.Vertex", pVertStr, 2);
	glswGetShadersAlt( "shaders.Shared+shaders.Skymap.Fragment", pFragStr, 2);
	CShader vertexShader(CShader::VERT, pVertStr, 2);
	CShader fragmentShader(CShader::FRAG, pFragStr, 2);	
	//setup the shaders
	program_cube.Initialise();
	program_cube.AddShader(&vertexShader);
	program_cube.AddShader(&fragmentShader);	
	program_cube.Link();

	program_cube.Start();
	program_cube.SetTextureUnit("cube_map",0);

	// - - - - - - - - - - - - - - - - - - - -
	// - - - - - - - - - - - - - - - - - - - -
	glswGetShadersAlt( "shaders.Shared+shaders.CubemapReflections.Vertex", pVertStr, 2);
	glswGetShadersAlt( "shaders.Shared+shaders.CubemapReflections.Fragment", pFragStr, 2);
	CShader vertexShader1(CShader::VERT, pVertStr, 2);
	CShader fragmentShader1(CShader::FRAG, pFragStr, 2);	
	//setup the shaders
	program_sphere.Initialise();
	program_sphere.AddShader(&vertexShader1);
	program_sphere.AddShader(&fragmentShader1);	
	program_sphere.Link();

	program_sphere.Start();
	program_sphere.SetTextureUnit("cube_map",0);

	// - - - - - - - - - - 
	//set up shapes
	//sphere = CreateShapeSphere();
	sphere = CreateShapeSphereNormals();
	cube = CreateShapeCubeNormals();
	box = CreateShapeCube();
	
	//sphere = CreateShapeCylinder();
	//sphere = CreateShapeCone();
	//sphere = CreateShapeTorus();

	// - - - - - - - - - -
	// set up cubemap
	CreateCubeMapTextures();

	//ubo for cameras etc
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	Transform::CreateProjectionMatrix(transforms.proj, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 50.0f);
}

void MainLoop(CPlatform * const  pPlatform)
{	//update the main application
	CVec3df cameraPosition( 0.0f, 0.0f, distance );

	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT);
	WRender::EnableDepthTest(true);

	WRender::ActiveTexture(WRender::Texture::UNIT_0);
	WRender::BindTexture(cubemap_tex);

	transform.Push();			
	{
		CMatrix44 rotLat,rotLong,inverse_camera, model_matrix;
		CVec3df camera_position(0,0,distance);

		rotLat.Rotate(latitude, 1, 0, 0);
		rotLong.Rotate(longitude, 0, 1, 0);

		transform.Translate(cameraPosition);
		transform.ApplyTransform(rotLat);
		transform.ApplyTransform(rotLong);			

		camera_position = transform.GetCurrentMatrix() * camera_position;
		inverse_camera = transform.GetCurrentMatrix();		
		inverse_camera = inverse_camera.InvertSimple();

		transform.Push();					
		{
			transform.Scale(10.0f,10.0f,10.0f);
			transforms.mv = transform.GetCurrentMatrix();
			transforms.mvp = transforms.proj * transforms.mv;
			CMatrix33 normal;

			normal = transforms.mv;
			normal = normal.Invert();
			normal = normal.Transpose();			
			transforms.nrm[ 0] = normal.x.x;			transforms.nrm[ 1] = normal.x.y;			transforms.nrm[ 2] = normal.x.z;
			transforms.nrm[ 4] = normal.y.x;			transforms.nrm[ 5] = normal.y.y;			transforms.nrm[ 6] = normal.y.z;
			transforms.nrm[ 8] = normal.z.x;			transforms.nrm[ 9] = normal.z.y;			transforms.nrm[10] = normal.z.z;

			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			WRender::EnableCulling(true);
			WRender::CullMode(WRender::FRONT_FACE);
			program_cube.Start();			
			DrawShape(box);
		}
		transform.Pop();

		transform_test.Identity();

		//transform_test.Push();
		transform.Push();					
		{
			//transform_test.Translate(0.0f, 0, 0);
			transform.Translate(0.0f, 0, 0);

			//model_matrix = transform_test.GetCurrentMatrix();

			transforms.mv = transform.GetCurrentMatrix();
			transforms.mvp = transforms.proj * transforms.mv;
			CMatrix33 normal;

			normal = transforms.mv;
			normal = normal.Invert();
			normal = normal.Transpose();			
			transforms.nrm[ 0] = normal.x.x;			transforms.nrm[ 1] = normal.x.y;			transforms.nrm[ 2] = normal.x.z;
			transforms.nrm[ 4] = normal.y.x;			transforms.nrm[ 5] = normal.y.y;			transforms.nrm[ 6] = normal.y.z;
			transforms.nrm[ 8] = normal.z.x;			transforms.nrm[ 9] = normal.z.y;			transforms.nrm[10] = normal.z.z;

			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			WRender::EnableCulling(true);			
			WRender::CullMode(WRender::BACK_FACE);
			program_sphere.Start();
			program_sphere.SetMtx44("inverse_view", inverse_camera.data);
			//program_sphere.SetMtx44("model_matrix", model_matrix.data);			
			DrawShape(sphere);
		}
		//transform_test.Pop();
		transform.Pop();
		
		//transform_test.Push();
		transform.Push();
		{
			static float rotation = 0;
			//transform_test.Translate(2.5f, 0, 0);
			transform.Translate(2.5f, 0, 0);
			CMatrix44 rotY;
			rotY.Rotate(rotation, 0, 1, 0);
			//transform_test.ApplyTransform(rotY);		
			transform.ApplyTransform(rotY);		
			rotation += 3.0f* pPlatform->GetDT();

			//model_matrix = transform_test.GetCurrentMatrix();

			transforms.mv = transform.GetCurrentMatrix();
			transforms.mvp = transforms.proj * transforms.mv;
			CMatrix33 normal;

			normal = transforms.mv;
			normal = normal.Invert();
			normal = normal.Transpose();			
			transforms.nrm[ 0] = normal.x.x;			transforms.nrm[ 1] = normal.x.y;			transforms.nrm[ 2] = normal.x.z;
			transforms.nrm[ 4] = normal.y.x;			transforms.nrm[ 5] = normal.y.y;			transforms.nrm[ 6] = normal.y.z;
			transforms.nrm[ 8] = normal.z.x;			transforms.nrm[ 9] = normal.z.y;			transforms.nrm[10] = normal.z.z;

			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
			WRender::EnableCulling(true);			
			WRender::CullMode(WRender::BACK_FACE);
			program_sphere.Start();
			program_sphere.SetMtx44("inverse_view",inverse_camera.data);
			//program_sphere.SetMtx44("model_matrix", model_matrix.data);
			DrawShape(cube);
		}
		//transform_test.Pop();
		transform.Pop();
		
	}
	transform.Pop();		

	pPlatform->UpdateBuffers();
	if (pPlatform->GetKeyboard().keys[KB_LEFTSHIFT].IsPressed()){
		if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed())
			distance += 1.0f * pPlatform->GetDT();
		if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed())
			distance -= 1.0f * pPlatform->GetDT();
	}else{
		if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed())
			latitude += 90.0f * pPlatform->GetDT();
		if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed())
			latitude -= 90.0f * pPlatform->GetDT();
		
		if(pPlatform->GetKeyboard().keys[KB_LEFT].IsPressed())//l
			longitude += 90.0f * pPlatform->GetDT();
		if(pPlatform->GetKeyboard().keys[KB_RIGHT].IsPressed())//r
			longitude -= 90.0f * pPlatform->GetDT();
	}
}

void CleanUp(void)
{	
	WRender::DeleteBuffer(ubo);
	DestroyShape(sphere);
	DestroyShape(cube);
	DestroyShape(box);
	WRender::DeleteTexture( cubemap_tex );	

	program_cube.CleanUp();
	program_sphere.CleanUp();

	glswShutdown();
}