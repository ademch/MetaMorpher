
#include "stdafx.h"
#include "GLSL_Pipeline.h"
#include "../../!!adGlobals/wdir.h"
#include "../../!!adGlobals/PNG/lodepng.h"
#include "../../!!adGlobals/JPG/JPEG_library.h"
#include "../../PS_SDK/libwebp-1.2.2/src/webp/webp_loader.h"

extern TextureBank  texBank;

namespace ImageSaveLoadHelper
{
	void _FlipImage(unsigned char* image, unsigned int width, unsigned int height)
	{

		for (unsigned int j = 0; j < height/2; j++)	// in case of odd rows, the central one is omitted
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

	unsigned char* LoadImageFromDisk(unsigned int &width, unsigned int &height)
	{
		HWND hWnd = WindowFromDC(wglGetCurrentDC());

		// common dialog box structure, setting all fields to 0 is important
		OPENFILENAME ofn  = { 0 };
		TCHAR szFile[260] = { 0 };

		// Initialize the fields of OPENFILENAME structure
		ofn.lStructSize     = sizeof(ofn);
		ofn.hwndOwner       = hWnd;
		ofn.lpstrFile       = szFile;
		ofn.nMaxFile        = sizeof(szFile);
		ofn.lpstrFilter     = _T("All(*.*)\0*.*\0Image(*.png;*.jpg;*.jpeg;*.webp)\0*.png;*.jpg;*.jpeg;*.webp\0\0");
		ofn.nFilterIndex    = 2;
		ofn.lpstrFileTitle  = NULL;
		ofn.nMaxFileTitle   = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileName(&ofn) == TRUE)
		{
			// use ofn.lpstrFile here
			TCHAR* strExt = GetFileExtension(ofn.lpstrFile);

			printf("Loading file...");

				unsigned char* image = NULL;

				if (_stricmp(strExt, ".png") == 0)
				{
					unsigned int error = lodepng_decode32_file(&image, &width, &height, ofn.lpstrFile);

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
					return NULL;
				}

				// libraries load bottom rows top, swapping is required
				_FlipImage(image, width, height);

			printf("done\n");

			return image;
		}
		
		return NULL;
	}


	bool SaveImageToDisk()
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
					_FlipImage(image, iWidth, iHeight);

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

}
