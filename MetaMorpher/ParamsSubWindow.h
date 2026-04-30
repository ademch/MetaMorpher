#ifndef PARAMSSUBSUBWINDOW_H
#define PARAMSSUBSUBWINDOW_H

#include "../../!!adGUI/SubWindowWithGUI.h"
#include "../../!!adGUI/Slider.h"
#include "../../!!adGUI/OnOffFlipSwitch.h"
#include "../../!!adGUI/button.h"
#include <vector>

class SliderG : public Slider<SL_INT>
{
public:
	SliderG(std::string strCaption, int x, int y, float _v_min, float _v_max, float* _v_cur, float scale) :
		     Slider(strCaption, x, y, _v_min, _v_max, _v_cur, scale) {};
	
	void Draw()
	{
		Slider::Draw();

		glFontBegin(&font);
			glFontTextOut("analog", posx + 20, posy +13, 5, 5);
			glFontTextOut("digital", posx + m_iBox_width/2.0f + 20, posy +13, 5, 5);
		glFontEnd();
		glDisable(GL_TEXTURE_2D);

		glColor3f(0.9,0.0,0.0);
		glBegin(GL_LINES);
			glVertex3f(posx + m_iBox_width / 2.0, posy, 5);
			glVertex3f(posx + m_iBox_width / 2.0, posy + _text_height, 5);
		glEnd();
	}
};

class SliderCenterLine : public Slider<SL_FLOAT>
{
public:
	SliderCenterLine(std::string strCaption, int x, int y, float _v_min, float _v_max, float* _v_cur, float scale) :
		      Slider(strCaption, x,y, _v_min,_v_max, _v_cur, scale) {};

	void Draw()
	{
		Slider::Draw();

		glColor3f(0.9, 0.0, 0.0);
		glBegin(GL_LINES);
			glVertex3f(posx + m_iBox_width / 10.0, posy, 5);
			glVertex3f(posx + m_iBox_width / 10.0, posy + _text_height, 5);
		glEnd();
	}
};

class ParamsSubWindow : public OpenGLSubWindowWithGUI
{
public:
	ParamsSubWindow(int iParentWidth, int iParentHeight,
					float fBottomLeftXperc, float fBottomLeftYperc,
					float fWidthPerc, float fHeightPerc);
	~ParamsSubWindow();

	bool LoadImageFromDisk();
	bool SaveFrame();

	bool StartNextGeneration();
	bool ShowOriginal();

	bool CompileShaders(bool bON);

	float fMorphPower;
	float fMorphRadius;
	float fMorphRatio;

	bool PointsAreVisible();
	bool ShowWireframe();
	void MakePointsVisible();

protected:

private:
	Slider<SL_INT>   *SliderMorphRatio;
	Slider<SL_INT>   *SliderMorphRadius;
	SliderCenterLine *SliderMorphSmoothness;
	
	OnOffFlipSwitch* onoffswitchConnect;
	OnOffFlipSwitch* onoffswitchWireframe;
	OnOffFlipSwitch* onoffswitchShowPoints;
	OnOffFlipSwitch* onoffpushbuttonOriginal;
	Button* buttonLoadImage;
	Button* buttonSaveFrame;
	Button* buttonMorphNext;

};


#endif