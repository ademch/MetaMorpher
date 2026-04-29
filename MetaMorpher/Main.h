#pragma once

//#include "resource.h"
#include "../../!!adGlobals/glut/glut.h"
#include "../../!!adGUI/glfont.h"
#include "../../!!adGUI/SubWindowWithGUI.h"
#include <vector>
#include "GLSL_Pipeline.h"
#include "MorphFBOprocessor.h"


GLFONT font;

int iAppWndWidth = 1200;
int iAppWndHeight = 800;


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

std::vector<OpenGLSubWindowWithGUI*> liWindows;

GLSL_Pipeline glsl_pipeline;

MorphFBOprocessor* fbo;
