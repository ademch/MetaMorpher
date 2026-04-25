#include "stdafx.h"
#include "ParamsSubWindow.h"
#include "../../!!adGlobals/adOpenGLUtilities.h"
#include "../../!!adGUI/fps.h"
#include "GLSL_Pipeline.h"
#include "../../!!adExtensions/extensions.h"


extern GLSL_Pipeline glsl_pipeline;
extern TextureBank texBank;

ParamsSubWindow::ParamsSubWindow(int iParentWidth, int iParentHeight,
								 float fBottomLeftXperc, float fBottomLeftYperc,
								 float fWidthPerc, float fHeightPerc) :
	      OpenGLSubWindowWithGUI(iParentWidth, iParentHeight,
								 fBottomLeftXperc, fBottomLeftYperc, fWidthPerc, fHeightPerc)
{
	fMorphRatio = 90;
	SliderMorphRatio = new Slider<SL_INT>(" Ratio", 30,210, 0,100, &fMorphRatio, 7);
	SliderMorphRatio->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	SliderMorphRatio->SetBoxWidth(200);
	SliderMorphRatio->SetBoxSeparation(1);
	SliderMorphRatio->fValueGranularity = 1;
	SliderMorphRatio->fTickGranularity = 10;
	liGUI_Elements.push_back(SliderMorphRatio);
	
	fMorphRadius = 80;
	SliderMorphRadius = new Slider<SL_INT>(" Radius", 30,170, 0,500, &fMorphRadius, 7);
	SliderMorphRadius->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	SliderMorphRadius->SetBoxWidth(200);
	SliderMorphRadius->SetBoxSeparation(1);
	SliderMorphRadius->fValueGranularity = 1;
	SliderMorphRadius->fTickGranularity = 30;
	liGUI_Elements.push_back(SliderMorphRadius);

	fMorphPower = 1.0f;
	SliderMorphSmoothness = new SliderCenterLine("Power", 30,130, 0.1, 10.1, &fMorphPower, 7);
	SliderMorphSmoothness->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	SliderMorphSmoothness->SetBoxWidth(200);
	SliderMorphSmoothness->SetBoxSeparation(1);
	SliderMorphSmoothness->fValueGranularity = 0.1;
	SliderMorphSmoothness->fTickGranularity = 0.5;
	liGUI_Elements.push_back(SliderMorphSmoothness);

	onoffswitchWireframe = new OnOffFlipSwitch("Wireframe", 30,90, 6);
	onoffswitchWireframe->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	liGUI_Elements.push_back(onoffswitchWireframe);

	OnOffFlipSwitch* onoffswitchShaders = new OnOffFlipSwitch("Shaders", 190,90, 6);
	onoffswitchShaders->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	//onoffswitchShaders->bEnabled = false;
	onoffswitchShaders->OnPreClick = [this](bool bON_Request) { return CompileShaders(bON_Request); };
	liGUI_Elements.push_back(onoffswitchShaders);

	onoffswitchShowPoints = new OnOffFlipSwitch("Points", 30,50, 6);
	onoffswitchShowPoints->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	onoffswitchShowPoints->bON = true;
	liGUI_Elements.push_back(onoffswitchShowPoints);

	onoffpushbuttonOriginal = new OnOffFlipSwitch("Original", 190,50, 6);
	onoffpushbuttonOriginal->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	onoffpushbuttonOriginal->bON = false;
	onoffpushbuttonOriginal->bPushButton = true;
	liGUI_Elements.push_back(onoffpushbuttonOriginal);

	buttonMorphNext = new Button("Apply", 30,-10, 120, 6.3);
	buttonMorphNext->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	buttonMorphNext->OnClick = [this]() { return StartNextGeneration(); };
	liGUI_Elements.push_back(buttonMorphNext);

	buttonLoadImage = new Button("Load image...", 30,-110, 120, 6.3);
	buttonLoadImage->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	buttonLoadImage->OnClick = [this]() { return LoadImageFromDisk(); };
	liGUI_Elements.push_back(buttonLoadImage);

	buttonSaveFrame = new Button("Save image...", 30,-140, 120, 6.3);
	buttonSaveFrame->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
	buttonSaveFrame->OnClick = [this]() { return SaveFrame(); };
	liGUI_Elements.push_back(buttonSaveFrame);

	FPS* fpsElement = new FPS(200,-140, 6);
	fpsElement->SetAlignment(HALIGN_LEFT, VALIGN_CENTER);
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

	RenderGUI();
}

void ParamsSubWindow::PassiveMotionFunc(int x, int y)
{
	OpenGLSubWindow::PassiveMotionFunc(x, y);

	if ((x > m_iBottomLeftX) && (x < m_iBottomLeftX + m_iWidth) &&
		(y > m_iBottomLeftY) && (y < m_iBottomLeftY + m_iHeight))
	{
		PassiveMotionFuncGUI(x, y);
	}
}

void ParamsSubWindow::MouseFunc(int button, int state, int x, int y)
{
	OpenGLSubWindow::MouseFunc(button, state, x, y);

	if (MouseFuncGUI(button, state, x, y)) return;
}


void ParamsSubWindow::MotionFunc(int x, int y)
{
	OpenGLSubWindow::MotionFunc(x, y);

	MotionFuncGUI(x, y);
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
