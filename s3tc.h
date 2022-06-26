// From: https://github.com/SoLaRGit/openS3TC
#pragma once
#include "stdafx.h"
#include <stdint.h>
#include <exception>

namespace S3TC
{
	class S3TC_Decompressor
	{
	public:
		uint32_t* image = nullptr;	// final output image

		S3TC_Decompressor(int w_, int h_, GLenum format_, unsigned char* compressed_pixels) : width(w_), height(h_), format(format_)
		{
			image = new uint32_t[w_ * h_ * sizeof(uint32_t)];
			if (format_ == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)			
				DXT1Decompress(compressed_pixels);
			else if (format_ == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)			
				DXT3Decompress(compressed_pixels);
			else if (format_ == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)			
				DXT5Decompress(compressed_pixels);
			else
				throw std::exception("ERROR! Unsupported .DDS compression format.");
		}

		~S3TC_Decompressor()
		{
			delete[] image;
		}

	private:
		int width = 0;
		int height = 0;
		GLenum format = 0;

		void DXT1Decompress(const unsigned char* blockStorage);
		void DXT3Decompress(const unsigned char* blockStorage);
		void DXT5Decompress(const unsigned char* blockStorage);
	};
}
