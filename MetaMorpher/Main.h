#pragma once

//#include "resource.h"
#include "../../!!adGlobals/glut/glut.h"
#include "../../!!adGUI/glfont.h"
#include "../../!!adGUI/SubWindow.h"
#include <vector>
#include "GLSL_Pipeline.h"
#include "MorphFBOprocessor.h"


GLFONT font;

int iAppWndWidth = 1300;
int iAppWndHeight = 480;


// объявления прототипов
void ReshapeFunc(GLsizei w, GLsizei h);
void globaldraw();
void keyboard(unsigned char, int, int);
void keyboardSpecial(int key, int x, int y);
void MouseFunc(int, int, int, int);
void MouseWheelFunc(int, int, int, int);
void MotionFunc(int x, int y);
void PassiveMotionFunc(int x, int y);
void WMClose();

static WNDPROC currentWndProc;
static HWND handle = NULL;
static LRESULT CALLBACK winProcUser(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

std::vector<OpenGLSubWindow*> liWindows;

GLSL_Pipeline glsl_pipeline;

MorphFBOprocessor* fbo;
