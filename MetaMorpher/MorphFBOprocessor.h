#ifndef MORPHFBOPROCESSORWINDOW_H
#define MORPHFBOPROCESSORWINDOW_H

#include "../../!!adGUI/SubWindow.h"
#include "../../!!adExtensions/Shaders.h"
#include <list>
#include "ParamsSubWindow.h"


class MorphFBOprocessor : public OpenGLSubWindow
{
public:
	MorphFBOprocessor(int iBottomLeftX, int iBottomLeftY, int iWidth, int iHeight);
	~MorphFBOprocessor();

	virtual	void Render();

	void _TextureUpdate_Test(int iWidth, int iHeight, int nrChannels);

	virtual bool KeyboardFunc(unsigned char key, int x, int y);

	FrameBufferObject* fbo;

	std::list<Vec2> mesh_list;

	ParamsSubWindow* m_ParamsSubWindow;

protected:
	bool LoadImageFromDisk();
	bool SaveImageToDisk();

private:
	TextureDescriptor* AllocFrameTexture(int iWidth, int iHeight, int nrChannels);
	TextureDescriptor* AllocFloatBufferTexture(int iWidth, int iHeight, int nrChannels);
	void ResizeTextures();

	void RenderFromMeshList(unsigned int tex, float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices);
	void GenMeshToList(float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices);
	void ShaderEmulate();

	void FlipImage(unsigned char* image, unsigned int width, unsigned int height);
};

#endif