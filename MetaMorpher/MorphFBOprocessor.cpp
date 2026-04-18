#include "stdafx.h"
#include "MorphFBOprocessor.h"
#include <gl/glut.h>
#include "GLSL_Pipeline.h"
#include "../../!!adExtensions/extensions.h"
#include "../../!!adGlobals/TextureDescriptor.h"
#include "../../!!adGlobals/wdir.h"
#include "../../!!adGlobals/PNG/lodepng.h"
#include "../../!!adGlobals/JPG/JPEG_library.h"
#include "../../PS_SDK/libwebp-1.2.2/src/webp/webp_loader.h"
#include "../../!!adGlobals/adOpenGLUtilities.h"


extern GLSL_Pipeline glsl_pipeline;
extern TextureBank  texBank;

MorphFBOprocessor::MorphFBOprocessor(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight) :
	                                 OpenGLSubWindow(iBottomLeftX, iBottomLeftY, iWidth, iHeight)
{
	bRenderGUIdecoration  = false;
	bSceneRotationAllowed = false;
	bSceneDragAllowed     = false;
	bSceneZoomAllowed     = false;

	m_ParamsSubWindow = NULL;

	clrBackground = Vecc3(0.2f, 0.2f, 0.2f);

	texBank.bank[TEXTURE_INPUT_IMAGE]  = AllocFrameTexture(iWidth, iHeight, 4);
	texBank.bank[TEXTURE_FLOAT_BUFFER] = AllocFloatBufferTexture(2, 2, 2);

	TextureDescriptor* texDesc;
	fbo = new FrameBufferObject(iWidth, iHeight);
	texDesc = fbo->Init();
	texBank.bank[TEXTURE_MORPHED_IMAGE] = texDesc;

}

void MorphFBOprocessor::ResizeTextures()
{
	glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_INPUT_IMAGE]->m_uiTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	texBank[TEXTURE_INPUT_IMAGE]->m_width  = m_iWidth;
	texBank[TEXTURE_INPUT_IMAGE]->m_height = m_iHeight;

	// FBO
	glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_MORPHED_IMAGE]->m_uiTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	texBank[TEXTURE_MORPHED_IMAGE]->m_width  = m_iWidth;
	texBank[TEXTURE_MORPHED_IMAGE]->m_height = m_iHeight;
}

MorphFBOprocessor::~MorphFBOprocessor()
{
	delete fbo;
}


void MorphFBOprocessor::GenMeshToList(float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices)
{
	mesh_list.clear();
	for (int j = 0; j<iVertSlices; j++) {
		for (int i = 0; i <= iHorSlices; i++)
		{
			Vec2 u01 = Vecc2(fX_bottom + fWidth*(i / float(iHorSlices)), fY_bottom + fHeight*((j + 1) / float(iVertSlices)));
			Vec2 u00 = Vecc2(fX_bottom + fWidth*(i / float(iHorSlices)), fY_bottom + fHeight* (j / float(iVertSlices)));

			//glVertex3f(u01.X, u01.Y, fZ);
			mesh_list.push_back(Vecc2(u01.X, u01.Y));

			//glVertex3f(u00.X, u00.Y, fZ);
			mesh_list.push_back(Vecc2(u00.X, u00.Y));
		}
	}
}

void MorphFBOprocessor::RenderFromMeshList(unsigned int tex, float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices)
{
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, tex);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glColor3f(0.0, 1.0, 0.0);

	for (int j = 0; j<iVertSlices; j++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i <= iHorSlices; i++)
		{
			Vec2 u01 = Vecc2(fX_bottom + fWidth*(i / float(iHorSlices)), fY_bottom + fHeight*((j + 1) / float(iVertSlices)));
			Vec2 u00 = Vecc2(fX_bottom + fWidth*(i / float(iHorSlices)), fY_bottom + fHeight* (j / float(iVertSlices)));

			glTexCoord2f((j + 1) / float(iVertSlices), i / float(iHorSlices));			
			Vec2 item = mesh_list.front();
			mesh_list.pop_front();
			glVertex3f(item.X, item.Y, fZ);

			glTexCoord2f(j / float(iVertSlices), i / float(iHorSlices));
			item = mesh_list.front();
			mesh_list.pop_front();
			glVertex3f(item.X, item.Y, fZ);
		}
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
}

void MorphFBOprocessor::ShaderEmulate()
{
	int iWidth  = texBank[TEXTURE_FLOAT_BUFFER]->m_width;
	int iHeight = texBank[TEXTURE_FLOAT_BUFFER]->m_height;
	int nrChannels = 2;
	Vec2* dataSrc = (Vec2 *)malloc(iWidth*iHeight*sizeof(float)*nrChannels);
	Vec2* dataDst = dataSrc+ iWidth;

	glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_FLOAT_BUFFER]->m_uiTextureID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, dataSrc);

	const float FLOAT_EPS = 0.001f;
	const float P_INFINITY = 1000000.0f;

	float fVotingRadius = 100;
	float fMorphRadius  = m_ParamsSubWindow->fMorphRadius;
	float fMorphPower   = m_ParamsSubWindow->fMorphPower;

	for (auto &vPosition : mesh_list)
	{
		int iMorphPointsCount = texBank[TEXTURE_FLOAT_BUFFER]->m_width;
		const float fMorphParameterA = 0.01;
		const float fMorphParameterB = 2.0f;

		float fTotalWeight = 0.0f;
		Vec2 vShift = Vecc2();
		for (int i = 0; i < iMorphPointsCount; i++)
		{
			Vec2 ptSrc = dataSrc[i];
			Vec2 ptDst = dataDst[i];

			float dist = PointDist(ptSrc, vPosition);
			float weight = 0;
			if (dist < fVotingRadius)
				weight = pow( cos( dist/fVotingRadius*PI)/2.0f + 0.5f, fMorphPower);//pow((fMorphParameterA + dist), -fMorphParameterB);
			fTotalWeight += weight;
			vShift = vShift + weight*(ptDst - ptSrc);
		}
		if (fTotalWeight > 0.001) {
			vShift = (1.0/fTotalWeight)*vShift;
		}

		// 2. During the second step it is decided the fullness of travel to the voted location
		float distMin = P_INFINITY;
		for (int i = 0; i < iMorphPointsCount; i++)
		{
			Vec2 ptSrc = dataSrc[i];

			float dist = PointDist(ptSrc, vPosition);
			if (dist < distMin)
				distMin = dist;
		}

		if (distMin < fMorphRadius)
		{
			float weight = pow( cos( distMin/fMorphRadius*PI)*0.5 + 0.5, 1.0);
			vShift = weight*vShift;
		}
		else
			vShift = Vecc2();

		vPosition = vPosition + vShift;

	} 
	free(dataSrc);

}



void MorphFBOprocessor::Render()
{
	//_TextureUpdate_Test(m_iWidth, m_iHeight, 4);

	if (!glsl_pipeline.bUseShaderPipeline) return;
	
	fbo->Activate();

	OpenGLSubWindow::Render();

	if (m_ParamsSubWindow->ShowWireframe())
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glLineWidth(1);

	glUseProgramObjectARB(glsl_pipeline.GPUPrograms["morph"]->programObj);
	glUniform1iARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("tex0"), 0);	// image
	glUniform1iARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("tex1"), 1);  // src/dst lines
	glUniform1fARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("fMorphRadius"), m_ParamsSubWindow->fMorphRadius);
	glUniform1fARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("fMorphPower"), m_ParamsSubWindow->fMorphPower);

		RenderTexturedQuadMesh(texBank[TEXTURE_INPUT_IMAGE]->m_uiTextureID,
			                   -m_iWidth/2.0f, -m_iHeight/2.0f, m_iWidth, m_iHeight, 0.0f, 110*float(m_iWidth)/float(m_iHeight), 110 );

	// if (bCPUShaderDebugging) {
	//	   GenMeshToList(-m_iWidth / 2.0f, -m_iHeight / 2.0f, m_iWidth, m_iHeight, 0.0, 30, 30);
	//     ShaderEmulate();
	//     RenderFromMeshList(texBank[TEXTURE_FLOAT_BUFFER]->m_uiTextureID,
	//                        -m_iWidth/2.0f, -m_iHeight/2.0f, m_iWidth, m_iHeight, 0.0, 30, 30);
	// }

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgramObjectARB(FFP);
	fbo->Deactivate();
}

TextureDescriptor* MorphFBOprocessor::AllocFrameTexture(int iWidth, int iHeight, int nrChannels)
{
	unsigned int iTexture;
	glGenTextures(1, &iTexture);
	glBindTexture(GL_TEXTURE_2D, iTexture);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// NPOT Texture!!! (supported since GL v2.0)
	unsigned char* data = NULL;
	data = (unsigned char *)malloc(iWidth*iHeight*nrChannels);
	//ZeroMemory(data, width*height*nrChannels);
	memset(data, 120, iWidth*iHeight*nrChannels);

	//           targ         mml  int frmt              brdr inc frmt   inc data type   inc data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iWidth, iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	free(data);

	return new TextureDescriptor(iTexture, iWidth, iHeight);
}

TextureDescriptor* MorphFBOprocessor::AllocFloatBufferTexture(int iWidth, int iHeight, int nrChannels)
{
	unsigned int iTexture;
	glGenTextures(1, &iTexture);
	glBindTexture(GL_TEXTURE_2D, iTexture);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// NPOT Texture!!! (supported since GL v2.0)
	float* data = (float *)malloc(iWidth*iHeight*sizeof(float)*nrChannels);
	ZeroMemory(data, iWidth*iHeight*sizeof(float)*nrChannels);
	//memset(data, 120, iWidth*iHeight*nrChannels);

	data[0] = 0.5f;  data[1] = 1.0f;
	data[2] = 0.7f;  data[3] = 0.2f;

	data[4] = 0.35f; data[5] = 0.1f;
	data[6] = 0.17f; data[7] = 0.28f;

	//           targ         mml  int frmt                brdr inc frmt inc data type   inc data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, iWidth, iHeight, 0, GL_RG, GL_FLOAT, data);
	glError();

	free(data);

	return new TextureDescriptor(iTexture, iWidth, iHeight);
}


void MorphFBOprocessor::_TextureUpdate_Test(int iWidth, int iHeight, int nrChannels)
{
	unsigned char* data = NULL;
	data = (unsigned char *)malloc(iWidth*iHeight*nrChannels);

	memset(data, 200, iWidth*iHeight*nrChannels);

	for (int j = 100; j < iHeight - 100; j++)
	{
		int odd = j % 2;
		for (int i = odd*nrChannels; i < iWidth*nrChannels; i += 2*nrChannels)	// skip every second
		{
			for (int k = 0; k < nrChannels; k++) {
				data[j*iWidth*nrChannels + i + k] = rand() % 255;
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_INPUT_IMAGE]->m_uiTextureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, iWidth,iHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);

	free(data);
}

void MorphFBOprocessor::FlipImage(unsigned char* image, unsigned int width, unsigned int height)
{

	for (unsigned int j = 0; j<height/2; j++)	// in case of odd rows, the central one is omitted
	{
		for (unsigned int i = 0; i < width * 4; i++)
		{
			// save bottom value
			unsigned char tmp = image[(height - j - 1)*width * 4 + i];
			image[(height - j - 1)*width * 4 + i] = image[j*width * 4 + i];
			image[j*width * 4 + i] = tmp;
		}
	}
}

bool MorphFBOprocessor::LoadImageFromDisk()
{
	HWND hWnd = WindowFromDC(wglGetCurrentDC());

	// common dialog box structure, setting all fields to 0 is important
	OPENFILENAME ofn  = { 0 };
	TCHAR szFile[260] = { 0 };

	// Initialize the fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("All(*.*)\0*.*\0Image(*.png;*.jpg;*.jpeg;*.webp)\0*.png;*.jpg;*.jpeg;*.webp\0\0");
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		// use ofn.lpstrFile here
		TCHAR* strExt = GetFileExtension(ofn.lpstrFile);

		printf("Loading file...");

			unsigned char* image = NULL;
			unsigned int width, height;

			if (_stricmp(strExt, ".png") == 0)
			{
				unsigned int error;
				error = lodepng_decode32_file(&image, &width, &height, ofn.lpstrFile);
				if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
			}
			else if ((_stricmp(strExt, ".jpg") == 0) || ((_stricmp(strExt, ".jpeg") == 0)))
			{
				read_JPEG_file(ofn.lpstrFile, &image, width, height);
			}
			else if (_stricmp(strExt, ".webp") == 0)
			{
				webP_loadImage(ofn.lpstrFile, &image, width, height);
			}
			else
			{
				printf("failed (%s file format not supported)\n", strExt);
				return false;
			}

			// libraries load bottom rows top, swapping is required
			FlipImage(image, width, height);

		printf("done\n");

		Reshape(0,0, width,height);

		ResizeTextures();

		glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_INPUT_IMAGE]->m_uiTextureID);
		//           targ         mml  int frmt             brdr inc frmt   inc data type   inc data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

		free(image);
	}

	return true;
}


bool MorphFBOprocessor::SaveImageToDisk()
{
	HWND hWnd = WindowFromDC(wglGetCurrentDC());

	// common dialog box structure, setting all fields to 0 is important
	OPENFILENAME ofn = { 0 };
	TCHAR szFile[260] = "Image.png\0";// { 0 };

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = _T("All(*.*)\0*.*\0Image(*.png)\0*.png\0\0");
	ofn.nFilterIndex = 2;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn) == TRUE) {
		// use ofn.lpstrFile here
		TCHAR* strExt = GetFileExtension(ofn.lpstrFile);

		printf("Saving file...");

		if (strcmp(strExt, ".png") == 0)
		{
			int iWidth  = texBank[TEXTURE_MORPHED_IMAGE]->m_width;
			int iHeight = texBank[TEXTURE_MORPHED_IMAGE]->m_height;
			int nrChannels = 4;
			unsigned char* image = (unsigned char *)malloc(iWidth*iHeight*nrChannels);

			glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_MORPHED_IMAGE]->m_uiTextureID);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

			// libraries load bottom rows top, swapping is required
			FlipImage(image, iWidth, iHeight);

			unsigned int error;
			error = lodepng_encode32_file(ofn.lpstrFile, image, iWidth, iHeight);
			if (error) printf("error %u: %s\n", error, lodepng_error_text(error));

			free(image);
		}
		else
		{
			printf("failed (%s file format not supported)\n", strExt);
			return false;
		}

		printf("done\n");
	}

	return true;
}


bool MorphFBOprocessor::KeyboardFunc(unsigned char key, int x, int y)
{
	bool res = false;

	if (OpenGLSubWindow::KeyboardFunc(key, x, y)) return true;

	switch (key)
	{
	case '2':
	{
		LoadImageFromDisk();
		break;
	}
	case '5':
	{
		SaveImageToDisk();
		break;
	}
	break;
	}

	return res;
}


