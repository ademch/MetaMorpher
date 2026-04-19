#ifndef MORPHFBOPROCESSORWINDOW_H
#define MORPHFBOPROCESSORWINDOW_H

#include "../../!!adGUI/SubWindow.h"
#include "../../!!adExtensions/Shaders.h"
#include <list>
#include "ParamsSubWindow.h"


class MorphFBOprocessor : public OpenGLSubWindow
{
public:
	FrameBufferObject* fbo;

	ParamsSubWindow* m_ParamsSubWindow;

	std::list<Vec2> mesh_listEmu;

	MorphFBOprocessor(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight);
	~MorphFBOprocessor();

	virtual	void Render();

	virtual void Reshape(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight);

	void _TextureUpdate_Test(int iWidth, int iHeight, int nrChannels);

	virtual bool KeyboardFunc(unsigned char key, int x, int y);

protected:

private:
	TextureDescriptor* AllocFrameTexture(int iWidth, int iHeight, int nrChannels);
	TextureDescriptor* AllocFloatBufferTexture(int iWidth, int iHeight, int nrChannels);

	void GenMeshToListEmu(float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices);
	void RenderFromMeshListEmu(unsigned int tex, float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices);
	void ShaderEmulate();

};

#endif