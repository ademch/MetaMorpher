#include "stdafx.h"
#include "ParamsSubWindow.h"
#include <gl/glut.h>
#include "../../!!adGlobals/adOpenGLUtilities.h"
#include "../../!!adGUI/fps.h"
#include "GLSL_Pipeline.h"
#include "../../!!adExtensions/extensions.h"


extern GLSL_Pipeline glsl_pipeline;
extern TextureBank texBank;

ParamsSubWindow::ParamsSubWindow(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight) :
			     OpenGLSubWindow(iBottomLeftX, iBottomLeftY, iWidth, iHeight)
{
	fMorphRadius = 80;
	SliderMorphRadius = new Slider<SL_INT>(" Radius", -m_iWidth/2 + 30, m_iHeight/2 - 120, 0,500, &fMorphRadius, 7);
	SliderMorphRadius->SetBoxWidth(200);
	SliderMorphRadius->SetBoxSeparation(1);
	SliderMorphRadius->fValueGranularity = 1;
	SliderMorphRadius->fTickGranularity = 30;
	liGUI_Elements.push_back(SliderMorphRadius);

	fMorphPower = 1.0f;
	SliderMorphSmoothness = new SliderCenterLine("Power", -m_iWidth/2 + 30, m_iHeight/2 - 160, 0.1, 10.1, &fMorphPower, 7);
	SliderMorphSmoothness->SetBoxWidth(200);
	SliderMorphSmoothness->SetBoxSeparation(1);
	SliderMorphSmoothness->fValueGranularity = 0.1;
	SliderMorphSmoothness->fTickGranularity = 0.5;
	liGUI_Elements.push_back(SliderMorphSmoothness);

	onoffswitchWireframe = new OnOffFlipSwitch("Wireframe", -m_iWidth/2 + 30, m_iHeight/2 - 200, 6);
	liGUI_Elements.push_back(onoffswitchWireframe);

	OnOffFlipSwitch* onoffswitchShaders = new OnOffFlipSwitch("Shaders", -m_iWidth/2 + 190, m_iHeight/2 - 200, 6);
	//onoffswitchShaders->bEnabled = false;
	onoffswitchShaders->OnPreClickThis = this;
	onoffswitchShaders->OnPreClick = (bool(__thiscall OpenGLSubWindow::*)(bool))&ParamsSubWindow::CompileShaders;
	liGUI_Elements.push_back(onoffswitchShaders);

	onoffswitchShowPoints = new OnOffFlipSwitch("Points", -m_iWidth/2 + 30, m_iHeight/2 - 240, 6);
	onoffswitchShowPoints->bON = true;
	liGUI_Elements.push_back(onoffswitchShowPoints);

	onoffpushbuttonOriginal = new OnOffFlipSwitch("Original", -m_iWidth / 2 + 190, m_iHeight / 2 - 240, 6);
	onoffpushbuttonOriginal->bON = false;
	onoffpushbuttonOriginal->bPushButton = true;
	liGUI_Elements.push_back(onoffpushbuttonOriginal);

	buttonMorphNext = new Button("Apply", -m_iWidth/2 + 30, m_iHeight/2 - 300, 120, 6.3);
	buttonMorphNext->OnClickThis = this;
	buttonMorphNext->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&ParamsSubWindow::StartNextGeneration;
	liGUI_Elements.push_back(buttonMorphNext);

	buttonLoadImage = new Button("Load image...", -m_iWidth/2 + 30, m_iHeight/2 - 400, 120, 6.3);
	buttonLoadImage->OnClickThis = this;
	buttonLoadImage->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&ParamsSubWindow::LoadImageFromDisk;
	liGUI_Elements.push_back(buttonLoadImage);

	buttonSaveFrame = new Button("Save image...", -m_iWidth/2 + 30, m_iHeight/2 - 430, 120, 6.3);
	buttonSaveFrame->OnClickThis = this;
	buttonSaveFrame->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&ParamsSubWindow::SaveFrame;
	liGUI_Elements.push_back(buttonSaveFrame);

	FPS* fpsElement = new FPS(-m_iWidth/2 + 200, m_iHeight/2 - 430, 6);
	liGUI_Elements.push_back(fpsElement);

	// Enable shaders

	onoffswitchShaders->SetOnOff(true, true);

}

ParamsSubWindow::~ParamsSubWindow()
{
	std::vector<GUI_Element*>::iterator iterElement;
	for (iterElement = liGUI_Elements.begin(); iterElement != liGUI_Elements.end(); iterElement++)
		delete (*iterElement);
}

void ParamsSubWindow::Render()
{
	OpenGLSubWindow::Render();

	glDisable(GL_LIGHTING);

	for (auto iterElement : liGUI_Elements)
		iterElement->Draw();
}

void ParamsSubWindow::PassiveMotionFunc(int x, int y)
{
	OpenGLSubWindow::PassiveMotionFunc(x, y);

	if ((x > m_iBottomLeftX) && (x < m_iBottomLeftX + m_iWidth) &&
		(y > m_iBottomLeftY) && (y < m_iBottomLeftY + m_iHeight))
	{
		SetupGraphicsPipeline();

		Vec3d v3DCoords;
		gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

		for (auto iterElement : liGUI_Elements)
			iterElement->Hover(v3DCoords.X, v3DCoords.Y);
	}
}

void ParamsSubWindow::MouseFunc(int button, int state, int x, int y)
{
	OpenGLSubWindow::MouseFunc(button, state, x, y);

	SetupGraphicsPipeline();

	Vec3d v3DCoords;
	gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

	for (auto iterElement : liGUI_Elements)
		iterElement->Clicked(button, state, v3DCoords.X, v3DCoords.Y);
}


void ParamsSubWindow::MotionFunc(int x, int y)
{
	OpenGLSubWindow::MotionFunc(x, y);

	SetupGraphicsPipeline();

	Vec3d v3DCoords;
	gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

	for (auto iterElement : liGUI_Elements)
		iterElement->Drag(v3DCoords.X, v3DCoords.Y);
}

bool ParamsSubWindow::LoadImageFromDisk()
{
	//get the HWND
	HWND handle = WindowFromDC(wglGetCurrentDC());
	PostMessage(handle, WM_KEYDOWN, 0x32, 0);	// 2

	return true;
}

bool ParamsSubWindow::StartNextGeneration()
{
	//get the HWND
	HWND handle = WindowFromDC(wglGetCurrentDC());
	PostMessage(handle, WM_KEYDOWN, 0x34, 0);	// 4

	return true;
}

bool ParamsSubWindow::ShowOriginal()
{
	return onoffpushbuttonOriginal->bON;
}


bool ParamsSubWindow::SaveFrame()
{
	//get the HWND
	HWND handle = WindowFromDC(wglGetCurrentDC());
	PostMessage(handle, WM_KEYDOWN, 0x35, 0);	// 5

	return true;
}

bool ParamsSubWindow::CompileShaders(bool bON_Request)
{
	if (!glsl_pipeline.bUseShaderPipeline)
	{
		assert(bON_Request);

		printf("Compiling shaders...\n");
			if (!glsl_pipeline.init_shaders()) return false;
		printf("done\n");

		glsl_pipeline.bUseShaderPipeline = true;
	}
	else
	{
		assert(!bON_Request);

		printf("Deleting shaders...\n");
			glsl_pipeline.destroy_shaders();
		printf("done\n");

		glsl_pipeline.bUseShaderPipeline = false;
	}

	return true;
}

bool ParamsSubWindow::PointsAreVisible()
{
	return onoffswitchShowPoints->bON;
}

void ParamsSubWindow::MakePointsVisible()
{
	onoffswitchShowPoints->bON = true;
}

bool ParamsSubWindow::ShowWireframe()
{
	return onoffswitchWireframe->bON;
}
