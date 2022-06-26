#include <iostream>
#include "s3tc.h"
#include "nv_dds.h"
#include <opencv2/opencv.hpp>
#include <GLFW/glfw3.h>

using namespace std;


void initOpenGL()
{
	if (glfwInit() != GLFW_TRUE) {
		throw std::exception("ERROR! Could not initialize the GLFW library.");
	}	

	// Create dummy window required to use OGL context.	
	unsigned int window_sz = 32;
	GLFWwindow* window = glfwCreateWindow(window_sz, window_sz,"S3TC Decompressor",nullptr, nullptr);
	if (window == nullptr) {
		glfwTerminate();
		throw std::exception("Failed to create window");
	}
	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		throw std::exception("Failed to initialize GLEW\n");
	}
}


GLenum formatFromInternalFormat(GLenum fmt) {
	switch (fmt)
	{
	case GL_R:
	case GL_RED:
	case GL_R8:
	case GL_R16:
	case GL_R16F:
	case GL_R32F:
	case GL_R8_SNORM:
	case GL_R16_SNORM:
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
		return GL_RED;

	case GL_R8UI:
	case GL_R8I:
	case GL_R16I:
	case GL_R16UI:
		return GL_RED_INTEGER;

	case GL_RG:
	case GL_RG8:
	case GL_RG16:
	case GL_RG16F:
	case GL_RG32F:
	case GL_RG_INTEGER:
	case GL_RG8_SNORM:
	case GL_RG16_SNORM:
	case GL_COMPRESSED_RG:
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
		return GL_RG;

	case GL_RG8UI:
	case GL_RG8I:
	case GL_RG16UI:
	case GL_RG16I:
		return GL_RG_INTEGER;

	case GL_RGB:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB5_A1:
	case GL_RGB10_A2:
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGB_INTEGER:
	case GL_RGB8_SNORM:
	case GL_RGB16_SNORM:
	case GL_COMPRESSED_SRGB:
	case GL_COMPRESSED_RGB:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	case GL_DOT3_RGB_EXT:
		return GL_RGB;


	case GL_RGB8UI:
	case GL_RGB8I:
	case GL_RGB16:
	case GL_RGB16UI:
	case GL_RGB16I:
		return GL_RGB_INTEGER;

	case GL_BGR:
	case GL_BGR_INTEGER:
		return GL_RGB;//<== NOTE: cannot use swapped bytes for texture format, 'GL_BGR' throws an error.

	case GL_RGBA:
	case GL_RGBA8:
	case GL_RGBA16F:
	case GL_RGBA32F:
	case GL_RGBA_INTEGER:
	case GL_RGBA8_SNORM:
	case GL_RGBA16_SNORM:
	case GL_COMPRESSED_RGBA:
	case GL_COMPRESSED_RGBA_BPTC_UNORM:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	case GL_DOT3_RGBA_EXT:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGBA12:
	case GL_COMPRESSED_SRGB_ALPHA:
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
	case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
	case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
	case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
	case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
	case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
	case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
	case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
	case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
	case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
	case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
	case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
	case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
	case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
	case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
	case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
	case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
		return GL_RGBA;

	case GL_RGBA8UI:
	case GL_RGBA8I:
	case GL_RGBA16:
	case GL_RGBA16UI:
	case GL_RGBA16I:
		return GL_RGBA_INTEGER;

	case GL_BGRA:
	case GL_BGRA_INTEGER:
		return GL_RGBA;//<== NOTE: cannot use swapped bytes for texture format, 'GL_BGRA' throws an error.
	}

	return GL_NONE;
}





int main(int argc, char **argv)
{
    if(argc != 2)
    {
        cout << "Usage: ./S3TC_Decompressor <input image path>" << endl;
        return 1;
    }

	// Initialize a dummy OpenGL context to use its enums and functions.
	initOpenGL();


	// ---------------------------------------------------------------------------

	// Use nv_dds to load compressed .dds image.
	std::string path(argv[1]);
	nv_dds::CDDSImage image;
	nv_dds::CSurface surf;	
	try
	{
		bool flipImage = false;
		image.load(path, flipImage);
	}
	catch (std::exception e1)
	{
		throw std::exception(e1.what());
	}
	int w = image.get_width();
	int h = image.get_height();
	int d = image.get_depth();
	int type = -1;
	int totalBytes = image.get_size();
	GLenum internalFormat = image.get_format();
	if (image.get_num_mipmaps() > 1)  // We only want first mipmap of image, the others we can auto-generate via OpenGL if we want to.
	{
		surf = image.get_mipmap(0);
		w = surf.get_width();
		h = surf.get_height();
		d = surf.get_depth();
		totalBytes = surf.get_size();
	}

	if (image.get_type() == nv_dds::TextureType::TextureFlat)
		type = GL_TEXTURE_2D;
	else
		throw std::exception("ERROR! Unsupported non-2D .dds file detected.");


	// Convert 'nv_dds' formatting from using swapped channel formats to something OpenGL can handle.
	switch (internalFormat) {
	case GL_BGRA_EXT:
		internalFormat = GL_RGBA;
		break;
	case GL_BGR_EXT:
		internalFormat = GL_RGB;
		break;
	case GL_LUMINANCE:
		internalFormat = GL_RED;
		break;
	}

	GLenum format = formatFromInternalFormat(internalFormat);
	int channels = format == GL_RGB ? 3 : 4;

	// Get pixel data.
	unsigned char* pixels = new unsigned char[totalBytes];
	unsigned int byte_counter = 0;
	if (image.get_num_mipmaps() > 1)
		memcpy(pixels, surf, surf.get_size());
	else
		memcpy(pixels, image, image.get_size());
	// Decompress data.
	{
		if (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT // Decompress S3TC format
			|| internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
			|| internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		{
			S3TC::S3TC_Decompressor s3tc(w, h, internalFormat, pixels);
			totalBytes = w * h * sizeof(uint32_t);
			delete[] pixels;
			pixels = new unsigned char[totalBytes];
			memcpy(pixels, s3tc.image, totalBytes);
			internalFormat = GL_RGBA;
		}
		else
		{
			throw std::exception("ERROR! Unsupported .DDS compression format.");
		}
	}


	// Display results.
	cv::Mat img = cv::Mat::zeros(w, h, channels == 3 ? CV_8UC3 : CV_8UC4);
	memcpy(img.data, pixels, totalBytes);
	cv::imshow("TEST", img);
	cv::waitKey(0);


	// Cleanup.
	img.deallocate();
	image.clear();
	delete[] pixels;

    return 0;
}
