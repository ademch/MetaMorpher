#include "stdafx.h"
#include "MorphFBOprocessor.h"
#include "GLSL_Pipeline.h"
#include "../../!!adExtensions/extensions.h"
#include "../../!!adGlobals/TextureDescriptor.h"
#include "../../!!adGlobals/adOpenGLUtilities.h"


extern GLSL_Pipeline glsl_pipeline;
extern TextureBank texBank;

MorphFBOprocessor::MorphFBOprocessor(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight) :
	                                 OpenGLSubWindow(iBottomLeftX, iBottomLeftY, iWidth, iHeight)
{
	bRenderGUIdecoration  = false;
	bSceneRotationAllowed = false;
	bSceneDragAllowed     = false;
	bSceneZoomAllowed     = false;

	m_ParamsSubWindow       = NULL;
	m_MorphingToolSubWindow = NULL;

	clrBackground = Vecc3(0.2f, 0.2f, 0.2f);

	texBank.bank[TEXTURE_INPUT_IMAGE]   = AllocFrameTexture(iWidth, iHeight, 4);
	texBank.bank[TEXTURE_FLOAT_BUFFER]  = AllocFloatBufferTexture(2, 2, 2);

	fbo = new FrameBufferObject(iWidth, iHeight);
	texBank.bank[TEXTURE_MORPHED_IMAGE] = fbo->Init();

}

void MorphFBOprocessor::Reshape(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight)
{
	OpenGLSubWindow::Reshape(iBottomLeftX,iBottomLeftY, iWidth,iHeight);

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


void MorphFBOprocessor::Render()
{
	//_TextureUpdate_Test(m_iWidth, m_iHeight, 4);

	if (!glsl_pipeline.bUseShaderPipeline) return;
	
	// Start rendering to frame buffer object
	fbo->Activate();

		OpenGLSubWindow::Render();

		if (m_ParamsSubWindow->ShowWireframe())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glLineWidth(1);

		glUseProgramObjectARB(glsl_pipeline.GPUPrograms["morph"]->programObj);

			glUniform1iARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("tex0"), 0);	// image
			glUniform1iARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("tex1"), 1);  // src/dst lines
			glUniform1fARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("fMorphRadius"), m_ParamsSubWindow->fMorphRadius);
			glUniform1fARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("fMorphPower"),  m_ParamsSubWindow->fMorphPower);
			glUniform1fARB(glsl_pipeline.GPUPrograms["morph"]->GetUniform("fMorphRatio"),  m_MorphingToolSubWindow->fMorphRatio);

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


bool MorphFBOprocessor::KeyboardFunc(unsigned char key, int x, int y)
{
	bool res = false;

	if (OpenGLSubWindow::KeyboardFunc(key, x, y)) return true;

	//switch (key)
	//{
	//	default:
	//	break;
	//}

	return res;
}


