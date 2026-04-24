#ifndef IMAGESAVELOAD_H
#define IMAGESAVELOAD_H

namespace ImageSaveLoadHelper
{
	unsigned char* LoadImageFromDisk(unsigned int &width, unsigned int &height);
	bool           SaveImageToDisk();

}

#endif
