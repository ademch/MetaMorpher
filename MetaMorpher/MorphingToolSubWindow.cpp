#include "stdafx.h"
#include "MorphingToolSubWindow.h"
#include "GLSL_Pipeline.h"
#include "../../!!adGUI/glfont.h"
#include "../../!!adExtensions/extensions.h"
#include "../../!!adGlobals/TextureDescriptor.h"
#include "../../!!adGlobals/adOpenGLUtilities.h"
#include "../../!!adGlobals/wdir.h"


extern GLSL_Pipeline glsl_pipeline;
extern TextureBank texBank;
extern GLFONT font;

const int   _fFinalizationRadius = 9;
const float const_fPointsDepth = 0.2;
const float const_fPointsSize = 7;
const float const_fLineWidth   = 2;

bool bDoubleClick = false;

void DoubleClickTimer(int value) {

	bDoubleClick = false;
}



MorphingToolSubWindow::MorphingToolSubWindow(int iParentWidth, int iParentHeight,
											 float fBottomLeftXperc, float fBottomLeftYperc,
											 float fWidthPerc, float fHeightPerc) :
	                   OpenGLSubWindowWithGUI(iParentWidth, iParentHeight,
											  fBottomLeftXperc, fBottomLeftYperc, fWidthPerc, fHeightPerc)
{
	stateCurrent = STATE_IDLE;
	bSrcCurveIsDone = false;
	bDstCurveIsDone = false;

	m_ParamsSubWindow = NULL;

	m_bIgnoreFalseInput = false;

	buttonSource = new Button("Draw src", -230,10, 100, 6.3);
	buttonSource->SetAlignment(HALIGN_CENTER, VALIGN_BOTTOM);
	buttonSource->OnClickThis = this;
	buttonSource->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&MorphingToolSubWindow::SourcePolylineClicked;
	liGUI_Elements.push_back(buttonSource);

	arrow = new Arrow("", -120,10 + 8, 30, 6.3);
	arrow->SetAlignment(HALIGN_CENTER, VALIGN_BOTTOM);
	liGUI_Elements.push_back(arrow);

	buttonDestination = new Button("Draw dst", -80,10, 100, 6.3);
	buttonDestination->SetAlignment(HALIGN_CENTER, VALIGN_BOTTOM);
	buttonDestination->OnClickThis = this;
	buttonDestination->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&MorphingToolSubWindow::DestinationPolylineClicked;
	liGUI_Elements.push_back(buttonDestination);

	buttonMorphNow = new Button("Morph now", 40,10, 100, 6.3);
	buttonMorphNow->SetAlignment(HALIGN_CENTER, VALIGN_BOTTOM);
	buttonMorphNow->OnClickThis = this;
	buttonMorphNow->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&MorphingToolSubWindow::MorphNow;
	liGUI_Elements.push_back(buttonMorphNow);

	comboBox = new ComboBox("Default", -180,10, 170, 6.3);
	comboBox->SetAlignment(HALIGN_RIGHT, VALIGN_BOTTOM);
	comboBox->bVisible = false;
	comboBox->bEnabled = false;
	liGUI_Elements.push_back(comboBox);

	buttonResetView = new Button("Reset view", -180,-30, 100, 6); 
	buttonResetView->SetAlignment(HALIGN_RIGHT, VALIGN_TOP);
	buttonResetView->OnClickThis = this;
	buttonResetView->OnClick = (bool(__thiscall OpenGLSubWindow::*)())&MorphingToolSubWindow::ResetView;
	liGUI_Elements.push_back(buttonResetView);

	typeTexBankIter iter;
	for (iter = texBank.bank.begin(); iter != texBank.bank.end(); ++iter)
		comboBox->items.push_back( iter->first.c_str() );

	comboBox->SetSelected(TEXTURE_MORPHED_IMAGE);

	ptPrevPoint = Vecc2();

}


MorphingToolSubWindow::~MorphingToolSubWindow()
{
	std::vector<GUI_Element*>::iterator iterElement;
	for (iterElement = liGUI_Elements.begin(); iterElement != liGUI_Elements.end(); iterElement++)
		delete (*iterElement);
}

void MorphingToolSubWindow::Render()
{
	OpenGLSubWindow::Render();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Show output of the shader, while invisible input texture holds original
	TextureDescriptor* texDescr = texBank[TEXTURE_MORPHED_IMAGE];
	if (m_ParamsSubWindow->ShowOriginal())
		texDescr = texBank[TEXTURE_INPUT_IMAGE];

	//std::string sSelected = comboBox->GetSelected();
	RenderTexturedQuad(texDescr->m_uiTextureID,
				  	  -texDescr->m_width/2, -texDescr->m_height/2,
					   texDescr->m_width, texDescr->m_height);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	sprintf(m_strCaption, "%s %5.0f%%", "Zoom", fUserScale*100.0f);

	if (m_ParamsSubWindow->PointsAreVisible())
	{
		// SOURCE
		//glEnable(GL_LINE_SMOOTH);
		glLineWidth(const_fLineWidth);
		glColor3f(0.83, 0.69, 0);
		std::vector<Vec2> listOutSrc;
		CatmullSubdivide(liSource, listOutSrc, 10);
		glBegin(GL_LINE_STRIP);
			for (auto element : listOutSrc) {
				glVertex3f(element.X, element.Y, 0.1);
			}
		glEnd();

		glPointSize(const_fPointsSize);
		glColor3f(0.93, 0.8, 0);
		glBegin(GL_POINTS);
			for (auto element : liSource) {
				glVertex3f(element.X, element.Y, const_fPointsDepth);
			}
		glEnd();

		// DESTINATION
		glLineWidth(const_fLineWidth);
		glColor3f(0.23, 0.71, 0);
		std::vector<Vec2> listOutDst;
		CatmullSubdivide(liDestination, listOutDst, 10);
		glBegin(GL_LINE_STRIP);
			for (auto element : listOutDst) {
				glVertex3f(element.X, element.Y, 0.1);
			}
		glEnd();

		glPointSize(const_fPointsSize);
		glColor3f(0.3, 0.8, 0);
		glBegin(GL_POINTS);
			for (auto element : liDestination) {
				glVertex3f(element.X, element.Y, const_fPointsDepth);
			}
		glEnd();

		glColor3f(1,0,0);
		glLineWidth(3);
		float fFinRadCorrected = _fFinalizationRadius / fUserScale;
		if ((stateCurrent == STATE_SOURCE_POINT_INPUT) && (liSource.size() > 0))
			DrawCircle(Vecc3(liSource.back(), 0.3), fFinRadCorrected, 20);
		if ((stateCurrent == STATE_DESTINATION_POINT_INPUT) && (liDestination.size() > 0))
			DrawCircle(Vecc3(liDestination.back(), 0.3), fFinRadCorrected, 20);

		// Highlight matching point in a source curve when adding points to destination
		if ((stateCurrent == STATE_DESTINATION_POINT_INPUT) &&
			(liDestination.size() > 0) && (liSource.size() > 0))
		{
			unsigned int indexLast = liDestination.size();
			if (indexLast <= liSource.size())
				DrawCircle(Vecc3(liSource[indexLast-1].X, liSource[indexLast-1].Y, 0.3), fFinRadCorrected, 20);
		}

	}
	///////

	if (SrcCurveIsDone()) {
		buttonSource->_text = "Clear Src";
		buttonSource->bEnabled = true;
	}
	if (DstCurveIsDone()) {
		buttonDestination->_text = "Clear Dst";
		buttonDestination->bEnabled = true;
	}

	///////

	RenderGUI();
}



void MorphingToolSubWindow::UploadMorphingLines()
{
	assert(liSource.size() == liDestination.size());

	std::vector<Vec2> listOutSrc;
	CatmullSubdivide(liSource, listOutSrc, 20);

	std::vector<Vec2> listOutDst;
	CatmullSubdivide(liDestination, listOutDst, 20);

	assert(listOutSrc.size() == listOutDst.size());

	glActiveTextureARB(GL_TEXTURE1);

		glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_FLOAT_BUFFER]->m_uiTextureID);
		texBank[TEXTURE_FLOAT_BUFFER]->m_width  = listOutSrc.size();
		texBank[TEXTURE_FLOAT_BUFFER]->m_height = 2;

		//           targ         mml  int frmt  w                  h brdr inc: frmt    type    data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, listOutSrc.size(), 2,   0,    GL_RG, GL_FLOAT, NULL);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, listOutSrc.size(), 1, GL_RG, GL_FLOAT, listOutSrc.data());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,1, listOutDst.size(), 1, GL_RG, GL_FLOAT, listOutDst.data());

	glActiveTextureARB(GL_TEXTURE0);
}


void MorphingToolSubWindow::PassiveMotionFunc(int x, int y)
{
	Vec3d v3DCoords;
	float fJitter = 5;

	OpenGLSubWindow::PassiveMotionFunc(x, y);

	if ((x > m_iBottomLeftX) && (x < m_iBottomLeftX + m_iWidth) &&
		(y > m_iBottomLeftY) && (y < m_iBottomLeftY + m_iHeight))
	{
		if (stateCurrent == STATE_IDLE)
		{
			SetupGraphicsPipeline();

			gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

			bool bHit = false;
			for (auto point : liDestination)
			{
				if (VecLengthSqr(v3DCoords - Vecc3d(point.X, point.Y, const_fPointsDepth)) < sqr(fJitter/fUserScale))
				{
					glutSetCursor(GLUT_CURSOR_TOP_SIDE);
					bHit = true;
					break;
				}
			}

			if (!bHit) glutSetCursor(GLUT_CURSOR_INHERIT);
		}
		
		PassiveMotionFuncGUI(x, y);
	}
}

// Mouse button callback
void MorphingToolSubWindow::MouseFunc(int button, int state, int x, int y)
{
	float fJitter       = 5;
	float fJitterLine   = 12;
	float fBlindZoneRad = 20;

	OpenGLSubWindow::MouseFunc(button, state, x, y);

	if ((x > m_iBottomLeftX) && (x < m_iBottomLeftX + m_iWidth) &&
		(y > m_iBottomLeftY) && (y < m_iBottomLeftY + m_iHeight))
	{
		// Process GUI first
		if (MouseFuncGUI(button, state, x,y)) return;


		// Process local logic

		// MouseMove does not have a way to get button state,
		// we need to react only on left button
		if ((button != GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) m_bIgnoreFalseInput = true;


		SetupGraphicsPipeline();

		Vec3d v3DCoords;
		gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

		// any click down from idle starts the drawing mode first
		if ( ((stateCurrent == STATE_SOURCE_DRAWING_INPUT) || (stateCurrent == STATE_DESTINATION_DRAWING_INPUT)) &&
			 (button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		{
			if (stateCurrent == STATE_SOURCE_DRAWING_INPUT)
				liSource.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));
			else if (stateCurrent == STATE_DESTINATION_DRAWING_INPUT)
				liDestination.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));

			glutSetCursor(GLUT_CURSOR_TOP_SIDE);
			ptPrevPoint = Vecc2(x, y);
		}
		// if button has been released in drawing mode there are two options:
		// * released at the same !starting! point- switch to point input mode
		// * released at the other point- input is done
		else if (((stateCurrent == STATE_SOURCE_DRAWING_INPUT) || (stateCurrent == STATE_DESTINATION_DRAWING_INPUT)) &&
			     (button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
		{
			if ((stateCurrent == STATE_SOURCE_DRAWING_INPUT) &&
				(liSource.size() == 1) && (PointDist(Vecc2(x, y), ptPrevPoint) < 5))
				stateCurrent = STATE_SOURCE_POINT_INPUT;
			else if ((stateCurrent == STATE_DESTINATION_DRAWING_INPUT) &&
				(liDestination.size() == 1) && (PointDist(Vecc2(x, y), ptPrevPoint) < 5))
				stateCurrent = STATE_DESTINATION_POINT_INPUT;
			else
			{
				if (stateCurrent == STATE_SOURCE_DRAWING_INPUT)
					bSrcCurveIsDone = true;
				else if (stateCurrent == STATE_DESTINATION_DRAWING_INPUT)
					bDstCurveIsDone = true;

				stateCurrent = STATE_IDLE;
			}
		}
		// if button has been pressed in point mode there are two options:
		// * pressed at the same point- input is done
		// * pressed at the other point- register input
		else if (((stateCurrent == STATE_SOURCE_POINT_INPUT) || (stateCurrent == STATE_DESTINATION_POINT_INPUT)) &&
			     (button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		{
			if (PointDist(Vecc2(x, y), ptPrevPoint) <= _fFinalizationRadius)
			{
				if (stateCurrent == STATE_SOURCE_POINT_INPUT)
					bSrcCurveIsDone = true;
				else if (stateCurrent == STATE_DESTINATION_POINT_INPUT)
					bDstCurveIsDone = true;

				stateCurrent = STATE_IDLE;
			}
			else
			{
				if (stateCurrent == STATE_SOURCE_POINT_INPUT)
					liSource.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));
				else if (stateCurrent == STATE_DESTINATION_POINT_INPUT)
					liDestination.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));

				ptPrevPoint = Vecc2(x, y);
			}
		}
		// 1. We enter here for recording of initial position of a point for drag
		// 2. We enter here to add additional point to a finished line
		else if ((stateCurrent == STATE_IDLE) && (button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		{
			if (bSrcCurveIsDone || bDstCurveIsDone)
			{
				for (auto& point : liDestination)
				{
					if (VecLengthSqr(v3DCoords - Vecc3d(point.X, point.Y, const_fPointsDepth)) < sqr(fJitter/fUserScale))
					{
						glutSetCursor(GLUT_CURSOR_TOP_SIDE);
						stateCurrent = STATE_POINT_DRAG;
						m_ptrDraggedPoint = &point;

						return;
					}
				}
			}

			// allow adding points only after both curves are done (disputable)
			if (bSrcCurveIsDone && bDstCurveIsDone)
			{
				for (unsigned int i = 0; i < liDestination.size() - 1; i++)
				{
					if ((PointDistSqr(Vecc2(v3DCoords), liDestination[i])   > sqr(fBlindZoneRad/fUserScale)) &&
						(PointDistSqr(Vecc2(v3DCoords), liDestination[i+1]) > sqr(fBlindZoneRad/fUserScale)))
					{
						Vec3 ptOut;
						if (PointDistToLineSegment(Vecc3(v3DCoords),
							                       Vecc3(liDestination[i]), Vecc3(liDestination[i+1]),
							                       ptOut) < (fJitterLine/fUserScale))
						{

							if (bDoubleClick) {
								// insert before second point
								liDestination.insert(liDestination.begin() + i + 1, Vecc2(v3DCoords.X, v3DCoords.Y));

								if (liSource.size() + 1 == liDestination.size())
								{
									liSource.insert(liSource.begin()+i + 1, (liSource[i] + liSource[i + 1]) / 2.0f);
									UploadMorphingLines();
								}

								bDoubleClick = false;
								return;
							}
							else {
								bDoubleClick = true;
								glutTimerFunc(250, DoubleClickTimer, 0);
								return;
							}
						}
					}
				}
				
			}
		}
		// 1. We enter here to remove a point from a finished line
		else if ((stateCurrent == STATE_IDLE) && (button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
		{
			// allow removing points only after both curves are done (disputable)
			if (bSrcCurveIsDone && bDstCurveIsDone)
			{
				if (liDestination.size() > 1)
				{
					for (unsigned int i = 0; i < liDestination.size(); i++)
					{
						if (PointDistSqr(Vecc2(v3DCoords), liDestination[i]) < sqr(fJitter / fUserScale))
						{

							if (bDoubleClick)
							{
								liDestination.erase(liDestination.begin() + i);

								if (liSource.size() - 1 == liDestination.size())
								{
									liSource.erase(liSource.begin() + i);
									UploadMorphingLines();
								}

								bDoubleClick = false;
								return;
							}
							else {
								bDoubleClick = true;
								glutTimerFunc(250, DoubleClickTimer, 0);
								return;
							}
						}
					}
				}
			}
		}

	}

	if ((button != GLUT_LEFT_BUTTON) && (state == GLUT_UP))
		m_bIgnoreFalseInput = false;
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP) && (stateCurrent == STATE_POINT_DRAG))
		stateCurrent = STATE_IDLE;
}


void MorphingToolSubWindow::MotionFunc(int x, int y)
{
	OpenGLSubWindow::MotionFunc(x, y);

	if ((x > m_iBottomLeftX) && (x < m_iBottomLeftX + m_iWidth) &&
		(y > m_iBottomLeftY) && (y < m_iBottomLeftY + m_iHeight))
	{
		// Process GUI first
		MotionFuncGUI(x, y);

		// Process local logic

		if (m_bIgnoreFalseInput) return;

		SetupGraphicsPipeline();

		Vec3d v3DCoords;
		gluUnProjectFriendly(x, y, 0, v3DCoords.X, v3DCoords.Y, v3DCoords.Z);

		if ((stateCurrent == STATE_SOURCE_DRAWING_INPUT) || (stateCurrent == STATE_DESTINATION_DRAWING_INPUT))
		{
			Vec2 vDir = Vecc2(x, y) - ptPrevPoint;
			float fLen = VecLength(vDir);
			float fRadius = 30;
			if (fLen > fRadius)
			{
				//Vec2 ptNew2d = ptPrevPoint + (1.0f - fRadius / fLen)*vDir;

				//Vec3d v3DNew;
				//gluUnProjectFriendly(ptNew2d.X, ptNew2d.Y, 0, v3DNew.X, v3DNew.Y, v3DNew.Z);

				//if (PointDist(ptPrevPoint, Vecc2(round(ptNew2d.X), round(ptNew2d.Y))) > 5)
				//{
				//	liSource.push_back(Vecc2(v3DNew.X, v3DNew.Y));
				//	ptPrevPoint = Vecc2(round(ptNew2d.X), round(ptNew2d.Y));
				//}

				if (stateCurrent == STATE_SOURCE_DRAWING_INPUT)
					liSource.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));
				else if (stateCurrent == STATE_DESTINATION_DRAWING_INPUT)
					liDestination.push_back(Vecc2(v3DCoords.X, v3DCoords.Y));

				ptPrevPoint = Vecc2(x, y);
			}
		}
		else if (stateCurrent == STATE_POINT_DRAG)
		{
			m_ptrDraggedPoint->X = v3DCoords.X;
			m_ptrDraggedPoint->Y = v3DCoords.Y;

			// both curves are done (condition of mouse down)
			if (liDestination.size() == liSource.size())
				UploadMorphingLines();
		}
	}
}

void MorphingToolSubWindow::ClearSourceLine() {
	liSource.clear();
	bSrcCurveIsDone = false;
	stateCurrent = STATE_IDLE;
	buttonSource->_text = "Draw src";
}

void MorphingToolSubWindow::ClearDestinationLine() {
	liDestination.clear();
	bDstCurveIsDone = false;
	stateCurrent = STATE_IDLE;
	buttonDestination->_text = "Draw dst";
}

bool MorphingToolSubWindow::ResetView()
{
	OpenGLSubWindow::ResetView();

	return true;
}


bool MorphingToolSubWindow::KeyboardFunc(unsigned char key, int x, int y)
{
	bool res = false;

	if (OpenGLSubWindow::KeyboardFunc(key, x, y)) return true;

	switch (key)
	{
	case '2':
	{
		ClearSourceLine();
		ClearDestinationLine();
		UploadMorphingLines();
		break;
	}
	case '4':
	{
		StartNextGeneration();
		break;
	}
	break;
	}

	return res;
}


bool MorphingToolSubWindow::SourcePolylineClicked()
{
	// Sequence here matters
	if (SrcCurveIsDone())
	{
		ClearSourceLine();
	}
	else
	if (stateCurrent == STATE_IDLE)
	{
		stateCurrent = STATE_SOURCE_DRAWING_INPUT;
		buttonSource->bEnabled = false;
		buttonSource->_text = "Drawing";

		// nice touch
		m_ParamsSubWindow->MakePointsVisible();
	}

	return true;
}


bool MorphingToolSubWindow::DestinationPolylineClicked()
{
	// Sequence here matters
	if (DstCurveIsDone())
	{
		ClearDestinationLine();
	}
	else
	if (stateCurrent == STATE_IDLE)
	{
		stateCurrent = STATE_DESTINATION_DRAWING_INPUT;
		buttonDestination->_text = "Drawing";

		// nice touch
		m_ParamsSubWindow->MakePointsVisible();
	}

	return true;
}

bool MorphingToolSubWindow::MorphNow()
{
	if (liSource.size() == liDestination.size())
		UploadMorphingLines();

	return true;
}


void MorphingToolSubWindow::StartNextGeneration()
{
	ClearSourceLine();
	ClearDestinationLine();

	// Apply current morph
	MorphNow();

	unsigned int idSrc = texBank[TEXTURE_MORPHED_IMAGE]->m_uiTextureID;
	unsigned int iWidthSrc  = texBank[TEXTURE_MORPHED_IMAGE]->m_width;
	unsigned int iHeightSrc = texBank[TEXTURE_MORPHED_IMAGE]->m_height;

	unsigned int idDst = texBank[TEXTURE_INPUT_IMAGE]->m_uiTextureID;
	unsigned int iWidthDst  = texBank[TEXTURE_INPUT_IMAGE]->m_width;
	unsigned int iHeightDst = texBank[TEXTURE_INPUT_IMAGE]->m_height;
	unsigned int nrChannels = 4;

	assert(iWidthSrc == iWidthDst);
	assert(iHeightSrc == iHeightDst);

	unsigned char* data = (unsigned char *)malloc(iWidthSrc*iHeightSrc*nrChannels);

	glBindTexture(GL_TEXTURE_2D, idSrc);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, idDst);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iWidthDst, iHeightDst, GL_RGBA, GL_UNSIGNED_BYTE, data);

	free(data);
}