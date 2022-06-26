// This software contains source code provided by NVIDIA Corporation.
// License: http://developer.download.nvidia.com/licenses/general_license.txt

///////////////////////////////////////////////////////////////////////////////
//
// Description:
//
// Loads DDS images (DXTC1, DXTC3, DXTC5, RGB (888, 888X), and RGBA (8888) are
// supported) for use in OpenGL. Image is flipped when its loaded as DX images
// are stored with different coordinate system. If file has mipmaps and/or
// cubemaps then these are loaded as well. Volume textures can be loaded as
// well but they must be uncompressed.
//
// When multiple textures are loaded (i.e a volume or cubemap texture),
// additional faces can be accessed using the array operator.
//
// The mipmaps for each face are also stored in a list and can be accessed like
// so: image.get_mipmap() (which accesses the first mipmap of the first
// image). To get the number of mipmaps call the get_num_mipmaps function for
// a given texture.
//
// Call the is_volume() or is_cubemap() function to check that a loaded image
// is a volume or cubemap texture respectively. If a volume texture is loaded
// then the get_depth() function should return a number greater than 1.
// Mipmapped volume textures and DXTC compressed volume textures are supported.
//
///////////////////////////////////////////////////////////////////////////////
//
// Update: 9/15/2003
//
// Added functions to create new image from a buffer of pixels. Added function
// to save current image to disk.
//
// Update: 6/11/2002
//
// Added some convenience functions to handle uploading textures to OpenGL. The
// following functions have been added:
//
//     bool upload_texture1D();
//     bool upload_texture2D(unsigned int imageIndex = 0, GLenum target = GL_TEXTURE_2D);
//     bool upload_textureRectangle();
//     bool upload_texture3D();
//     bool upload_textureCubemap();
//
// See function implementation below for instructions/comments on using each
// function.
//
// The open function has also been updated to take an optional second parameter
// specifying whether the image should be flipped on load. This defaults to
// true.
//
///////////////////////////////////////////////////////////////////////////////
// Sample usage
///////////////////////////////////////////////////////////////////////////////
//
// Loading a compressed texture:
//
// CDDSImage image;
// GLuint texobj;
//
// image.load("compressed.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, image.get_format(),
//     image.get_width(), image.get_height(), 0, image.get_size(),
//     image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     CSurface mipmap = image.get_mipmap(i);
//
//     glCompressedTexImage2DARB(GL_TEXTURE_2D, i+1, image.get_format(),
//         mipmap.get_width(), mipmap.get_height(), 0, mipmap.get_size(),
//         mipmap);
// }
///////////////////////////////////////////////////////////////////////////////
//
// Loading an uncompressed texture:
//
// CDDSImage image;
// GLuint texobj;
//
// image.load("uncompressed.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glTexImage2D(GL_TEXTURE_2D, 0, image.get_components(), image.get_width(),
//     image.get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage2D(GL_TEXTURE_2D, i+1, image.get_components(),
//         image.get_mipmap(i).get_width(), image.get_mipmap(i).get_height(),
//         0, image.get_format(), GL_UNSIGNED_BYTE, image.get_mipmap(i));
// }
//
///////////////////////////////////////////////////////////////////////////////
//
// Loading an uncompressed cubemap texture:
//
// CDDSImage image;
// GLuint texobj;
// GLenum target;
//
// image.load("cubemap.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_CUBE_MAP);
// glBindTexture(GL_TEXTURE_CUBE_MAP, texobj);
//
// for (int n = 0; n < 6; n++)
// {
//     target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+n;
//
//     glTexImage2D(target, 0, image.get_components(), image[n].get_width(),
//         image[n].get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE,
//         image[n]);
//
//     for (int i = 0; i < image[n].get_num_mipmaps(); i++)
//     {
//         glTexImage2D(target, i+1, image.get_components(),
//             image[n].get_mipmap(i).get_width(),
//             image[n].get_mipmap(i).get_height(), 0,
//             image.get_format(), GL_UNSIGNED_BYTE, image[n].get_mipmap(i));
//     }
// }
//
///////////////////////////////////////////////////////////////////////////////
//
// Loading a volume texture:
//
// CDDSImage image;
// GLuint texobj;
//
// image.load("volume.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_3D);
// glBindTexture(GL_TEXTURE_3D, texobj);
//
// PFNGLTEXIMAGE3DPROC glTexImage3D;
// glTexImage3D(GL_TEXTURE_3D, 0, image.get_components(), image.get_width(),
//     image.get_height(), image.get_depth(), 0, image.get_format(),
//     GL_UNSIGNED_BYTE, image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage3D(GL_TEXTURE_3D, i+1, image.get_components(),
//         image[0].get_mipmap(i).get_width(),
//         image[0].get_mipmap(i).get_height(),
//         image[0].get_mipmap(i).get_depth(), 0, image.get_format(),
//         GL_UNSIGNED_BYTE, image[0].get_mipmap(i));
// }

// LIBRARY MODIFIED BY JH STROM 9/7/2021 to handle DX10 textures as well




#include "nv_dds.h"
#include <d3d11.h>
#include <cstring>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <dxgiformat.h>


using namespace std;
using namespace nv_dds;

#define GL_BGR_EXT                                        0x80E0
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3

///////////////////////////////////////////////////////////////////////////////
// CDDSImage private functions

namespace {
    // surface description flags
    const uint32_t DDSF_CAPS = 0x00000001;
    const uint32_t DDSF_HEIGHT = 0x00000002;
    const uint32_t DDSF_WIDTH = 0x00000004;
    const uint32_t DDSF_PITCH = 0x00000008;
    const uint32_t DDSF_PIXELFORMAT = 0x00001000;
    const uint32_t DDSF_MIPMAPCOUNT = 0x00020000;
    const uint32_t DDSF_LINEARSIZE = 0x00080000;
    const uint32_t DDSF_DEPTH = 0x00800000;

    // pixel format flags
    const uint32_t DDSF_ALPHAPIXELS = 0x00000001;
    const uint32_t DDSF_FOURCC = 0x00000004;
    const uint32_t DDSF_RGB = 0x00000040;
    const uint32_t DDSF_RGBA = 0x00000041;
#define DDSF_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDSF_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDSF_ALPHA       0x00000002  // DDPF_ALPHA
#define DDSF_PAL8        0x00000020  // DDPF_PALETTEINDEXED8

    // dwCaps1 flags
    const uint32_t DDSF_COMPLEX = 0x00000008;
    const uint32_t DDSF_TEXTURE = 0x00001000;
    const uint32_t DDSF_MIPMAP = 0x00400000;

    // dwCaps2 flags
    const uint32_t DDSF_CUBEMAP = 0x00000200;
    const uint32_t DDSF_CUBEMAP_POSITIVEX = 0x00000400;
    const uint32_t DDSF_CUBEMAP_NEGATIVEX = 0x00000800;
    const uint32_t DDSF_CUBEMAP_POSITIVEY = 0x00001000;
    const uint32_t DDSF_CUBEMAP_NEGATIVEY = 0x00002000;
    const uint32_t DDSF_CUBEMAP_POSITIVEZ = 0x00004000;
    const uint32_t DDSF_CUBEMAP_NEGATIVEZ = 0x00008000;
    const uint32_t DDSF_CUBEMAP_ALL_FACES = 0x0000FC00;
    const uint32_t DDSF_VOLUME = 0x00200000;

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32)(byte)(ch0) | ((uint32)(byte)(ch1) << 8) |       \
                ((uint32)(byte)(ch2) << 16) | ((uint32)(byte)(ch3) << 24))
#endif /* defined(MAKEFOURCC) */

    // compressed texture types
#define FOURCC_ATI1 MAKEFOURCC('A','T','I','1')
#define FOURCC_ATI2 MAKEFOURCC('A','T','I','2')
#define FOURCC_BC3U MAKEFOURCC('B','C','3','U')
#define FOURCC_BC4U MAKEFOURCC('B','C','4','U')
#define FOURCC_BC4S MAKEFOURCC('B','C','4','S')
#define FOURCC_BC5U MAKEFOURCC('B','C','5','U')
#define FOURCC_BC5S MAKEFOURCC('B','C','5','S')
#define FOURCC_DXT1 MAKEFOURCC('D','X','T','1')
#define FOURCC_DXT2 MAKEFOURCC('D','X','T','2')
#define FOURCC_DXT3 MAKEFOURCC('D','X','T','3')
#define FOURCC_DXT4 MAKEFOURCC('D','X','T','4')
#define FOURCC_DXT5 MAKEFOURCC('D','X','T','5')
#define FOURCC_DX10 MAKEFOURCC('D','X','1','0')
#define FOURCC_RGBG MAKEFOURCC('R','G','B','G')
#define FOURCC_GRGB MAKEFOURCC('G','R','B','G')

    struct DDS_PIXELFORMAT {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    };

    struct DDS_HEADER {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwPitchOrLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipMapCount;
        uint32_t dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t dwCaps1;
        uint32_t dwCaps2;
        uint32_t dwReserved2[3];
    };

    struct DDS_HEADER_DXT10
    {
        DXGI_FORMAT dxgiFormat;
        uint32_t    resourceDimension;
        uint32_t    miscFlag; // See D3D11_RESOURCE_MISC_FLAG
        uint32_t    arraySize;
        uint32_t    reserved;
    };

    size_t BitsPerPixel(_In_ DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 32;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
            return 8;

        case DXGI_FORMAT_R1_UNORM:
            return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;

        default:
            return 0;
        }
    }



#define ISBITMASK(r, g, b, a) (ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a)
    DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
    {
        if (ddpf.dwFlags & DDSF_RGB)
        {
            // Note that sRGB formats are written using the "DX10" extended header.

            switch (ddpf.dwRGBBitCount)
            {
            case 32:
                if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
                {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8

                // Note that many common DDS reader/writers (including D3DX) swap the
                // the RED/BLUE masks for 10:10:10:2 formats. We assumme
                // below that the 'backwards' header mask is being used since it is most
                // likely written by D3DX. The more robust solution is to use the 'DX10'
                // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

                // For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data
                if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                {
                    return DXGI_FORMAT_R10G10B10A2_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10

                if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16G16_UNORM;
                }

                if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
                {
                    // Only 32-bit color channel format in D3D9 was R32F.
                    return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114.
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
                {
                    return DXGI_FORMAT_B5G5R5A1_UNORM;
                }
                if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
                {
                    return DXGI_FORMAT_B5G6R5_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5.
                if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
                {
                    return DXGI_FORMAT_B4G4R4A4_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4.

                // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
                break;
            }
        }
        else if (ddpf.dwFlags & DDSF_LUMINANCE)
        {
            if (8 == ddpf.dwRGBBitCount)
            {
                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4.
            }

            if (16 == ddpf.dwRGBBitCount)
            {
                if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension.
                }
                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension.
                }
            }
        }
        else if (ddpf.dwFlags & DDSF_ALPHA)
        {
            if (8 == ddpf.dwRGBBitCount)
            {
                return DXGI_FORMAT_A8_UNORM;
            }
        }
        else if (ddpf.dwFlags & DDSF_FOURCC)
        {
            if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC1_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            // While pre-mulitplied alpha isn't directly supported by the DXGI formats,
            // they are basically the same as these BC formats so they can be mapped
            if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC4_SNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_BC5_SNORM;
            }

            // BC6H and BC7 are written using the "DX10" extended header

            if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_R8G8_B8G8_UNORM;
            }
            if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.dwFourCC)
            {
                return DXGI_FORMAT_G8R8_G8B8_UNORM;
            }

            // Check for D3DFORMAT enums being set here.
            switch (ddpf.dwFourCC)
            {
            case 36: // D3DFMT_A16B16G16R16
                return DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    static size_t GetNumBytes(
        _In_ size_t width,
        _In_ size_t height,
        _In_ DXGI_FORMAT fmt
    )
    {
        size_t numBytes = 0;
        size_t rowBytes = 0;
        size_t numRows = 0;

        bool bc = false;
        bool packed = false;
        size_t bcnumBytesPerBlock = 0;
        switch (fmt)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc = true;
            bcnumBytesPerBlock = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc = true;
            bcnumBytesPerBlock = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            packed = true;
            break;
        }

        if (bc)
        {
            size_t numBlocksWide = 0;
            if (width > 0)
            {
                numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
            }
            size_t numBlocksHigh = 0;
            if (height > 0)
            {
                numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
            }
            rowBytes = numBlocksWide * bcnumBytesPerBlock;
            numRows = numBlocksHigh;
        }
        else if (packed)
        {
            rowBytes = ((width + 1) >> 1) * 4;
            numRows = height;
        }
        else
        {
            size_t bpp = BitsPerPixel(fmt);
            rowBytes = (width * bpp + 7) / 8; // Round up to the nearest byte.
            numRows = height;
        }

        return rowBytes * numRows;
    }

    GLenum DX10formatToGL(_In_ DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return GL_RGBA32F;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return GL_RGBA32F;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return GL_RGBA32UI;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return GL_RGBA32I;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return GL_RGB32F;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return GL_RGB32F;
        case DXGI_FORMAT_R32G32B32_UINT:
            return GL_RGB32UI;
        case DXGI_FORMAT_R32G32B32_SINT:
            return GL_RGBA32I;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return GL_RGBA16F;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return GL_RGBA16F;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return GL_RGBA16;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return GL_RGBA16UI;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return GL_RGBA16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return GL_RGBA16I;

        case DXGI_FORMAT_R32G32_TYPELESS:
            return GL_RG32F;
        case DXGI_FORMAT_R32G32_FLOAT:
            return GL_RG32F;
        case DXGI_FORMAT_R32G32_UINT:
            return GL_RG32UI;
        case DXGI_FORMAT_R32G32_SINT:
            return GL_RG32I;

        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return GL_NONE;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return GL_NONE;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return GL_RGBA8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return GL_RGBA8;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return GL_RGBA8;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return GL_RGBA8UI;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return GL_RGBA8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return GL_RGBA8I;

        case DXGI_FORMAT_R16G16_TYPELESS:
            return GL_RG16F;
        case DXGI_FORMAT_R16G16_FLOAT:
            return GL_RG16F;
        case DXGI_FORMAT_R16G16_UNORM:
            return GL_RG16;
        case DXGI_FORMAT_R16G16_UINT:
            return GL_RG16UI;
        case DXGI_FORMAT_R16G16_SNORM:
            return GL_RG16_SNORM;
        case DXGI_FORMAT_R16G16_SINT:
            return GL_RG16I;

        case DXGI_FORMAT_R32_TYPELESS:
            return GL_R32F;
        case DXGI_FORMAT_D32_FLOAT:
            return GL_DEPTH_COMPONENT32F;
        case DXGI_FORMAT_R32_FLOAT:
            return GL_R32F;
        case DXGI_FORMAT_R32_UINT:
            return GL_R32UI;
        case DXGI_FORMAT_R32_SINT:
            return GL_R32I;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return GL_NONE;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
            return GL_RG8;
        case DXGI_FORMAT_R8G8_UINT:
            return GL_RG8UI;
        case DXGI_FORMAT_R8G8_SNORM:
            return GL_RG8_SNORM;
        case DXGI_FORMAT_R8G8_SINT:
            return GL_RG8I;
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
            return GL_R16F;
        case DXGI_FORMAT_D16_UNORM:
            return GL_DEPTH_COMPONENT16;
        case DXGI_FORMAT_R16_UNORM:
            return GL_R16;
        case DXGI_FORMAT_R16_UINT:
            return GL_R16UI;
        case DXGI_FORMAT_R16_SNORM:
            return GL_R16_SNORM;
        case DXGI_FORMAT_R16_SINT:
            return GL_R16I;

        case DXGI_FORMAT_B5G6R5_UNORM:
            return GL_NONE;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return GL_RGB5_A1;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return GL_NONE;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
            return GL_R8;
        case DXGI_FORMAT_R8_UINT:
            return GL_R8UI;
        case DXGI_FORMAT_R8_SNORM:
            return GL_R8I;
        case DXGI_FORMAT_R8_SINT:
            return GL_R8I;
        case DXGI_FORMAT_A8_UNORM:
            return GL_ALPHA8;

        case DXGI_FORMAT_R1_UNORM:
            return GL_NONE;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;

        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;

        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
            return GL_COMPRESSED_RED_RGTC1;
        case DXGI_FORMAT_BC4_SNORM:
            return GL_COMPRESSED_SIGNED_RED_RGTC1;

        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
            return GL_COMPRESSED_RG_RGTC2;
        case DXGI_FORMAT_BC5_SNORM:
            return GL_COMPRESSED_SIGNED_RG_RGTC2;

        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
            return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case DXGI_FORMAT_BC6H_SF16:
            return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;

        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
            return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT;

        default:
            return GL_NONE;
        }
    }

    static void GetSurfaceInfo(
        _In_ size_t width,
        _In_ size_t height,
        _In_ DXGI_FORMAT fmt,
        _Out_opt_ size_t* outNumBytes,
        _Out_opt_ size_t* outRowBytes,
        _Out_opt_ size_t* outNumRows
    )
    {
        size_t numBytes = 0;
        size_t rowBytes = 0;
        size_t numRows = 0;

        bool bc = false;
        bool packed = false;
        size_t bcnumBytesPerBlock = 0;
        switch (fmt)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc = true;
            bcnumBytesPerBlock = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc = true;
            bcnumBytesPerBlock = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            packed = true;
            break;
        }

        if (bc)
        {
            size_t numBlocksWide = 0;
            if (width > 0)
            {
                numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
            }
            size_t numBlocksHigh = 0;
            if (height > 0)
            {
                numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
            }
            rowBytes = numBlocksWide * bcnumBytesPerBlock;
            numRows = numBlocksHigh;
        }
        else if (packed)
        {
            rowBytes = ((width + 1) >> 1) * 4;
            numRows = height;
        }
        else
        {
            size_t bpp = BitsPerPixel(fmt);
            rowBytes = (width * bpp + 7) / 8; // Round up to the nearest byte.
            numRows = height;
        }

        numBytes = rowBytes * numRows;
        if (outNumBytes)
        {
            *outNumBytes = numBytes;
        }
        if (outRowBytes)
        {
            *outRowBytes = rowBytes;
        }
        if (outNumRows)
        {
            *outNumRows = numRows;
        }
    }

    string fourcc(uint32_t enc) {
        char c[5] = { '\0' };
        c[0] = enc >> 0 & 0xFF;
        c[1] = enc >> 8 & 0xFF;
        c[2] = enc >> 16 & 0xFF;
        c[3] = enc >> 24 & 0xFF;
        return c;
    }

    struct DXTColBlock {
        uint16_t col0;
        uint16_t col1;

        uint8_t row[4];
    };

    struct DXT3AlphaBlock {
        uint16_t row[4];
    };

    struct DXT5AlphaBlock {
        uint8_t alpha0;
        uint8_t alpha1;

        uint8_t row[6];
    };

    ///////////////////////////////////////////////////////////////////////////////
    // flip a DXT1 color block
    void flip_blocks_dxtc1(DXTColBlock* line, unsigned int numBlocks) {
        DXTColBlock* curblock = line;

        for (unsigned int i = 0; i < numBlocks; i++) {
            std::swap(curblock->row[0], curblock->row[3]);
            std::swap(curblock->row[1], curblock->row[2]);

            curblock++;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flip a DXT3 color block
    void flip_blocks_dxtc3(DXTColBlock* line, unsigned int numBlocks) {
        DXTColBlock* curblock = line;
        DXT3AlphaBlock* alphablock;

        for (unsigned int i = 0; i < numBlocks; i++) {
            alphablock = (DXT3AlphaBlock*)curblock;

            std::swap(alphablock->row[0], alphablock->row[3]);
            std::swap(alphablock->row[1], alphablock->row[2]);

            curblock++;

            std::swap(curblock->row[0], curblock->row[3]);
            std::swap(curblock->row[1], curblock->row[2]);

            curblock++;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flip a DXT5 alpha block
    void flip_dxt5_alpha(DXT5AlphaBlock* block) {
        uint8_t gBits[4][4];

        const uint32_t mask = 0x00000007;          // bits = 00 00 01 11
        uint32_t bits = 0;
        memcpy(&bits, &block->row[0], sizeof(uint8_t) * 3);

        gBits[0][0] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[0][1] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[0][2] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[0][3] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[1][0] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[1][1] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[1][2] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[1][3] = (uint8_t)(bits & mask);

        bits = 0;
        memcpy(&bits, &block->row[3], sizeof(uint8_t) * 3);

        gBits[2][0] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[2][1] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[2][2] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[2][3] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[3][0] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[3][1] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[3][2] = (uint8_t)(bits & mask);
        bits >>= 3;
        gBits[3][3] = (uint8_t)(bits & mask);

        uint32_t* pBits = ((uint32_t*)&(block->row[0]));

        *pBits = *pBits | (gBits[3][0] << 0);
        *pBits = *pBits | (gBits[3][1] << 3);
        *pBits = *pBits | (gBits[3][2] << 6);
        *pBits = *pBits | (gBits[3][3] << 9);

        *pBits = *pBits | (gBits[2][0] << 12);
        *pBits = *pBits | (gBits[2][1] << 15);
        *pBits = *pBits | (gBits[2][2] << 18);
        *pBits = *pBits | (gBits[2][3] << 21);

        pBits = ((uint32_t*)&(block->row[3]));

#ifdef MACOS
        * pBits &= 0x000000ff;
#else
        * pBits &= 0xff000000;
#endif

        * pBits = *pBits | (gBits[1][0] << 0);
        *pBits = *pBits | (gBits[1][1] << 3);
        *pBits = *pBits | (gBits[1][2] << 6);
        *pBits = *pBits | (gBits[1][3] << 9);

        *pBits = *pBits | (gBits[0][0] << 12);
        *pBits = *pBits | (gBits[0][1] << 15);
        *pBits = *pBits | (gBits[0][2] << 18);
        *pBits = *pBits | (gBits[0][3] << 21);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flip a DXT5 color block
    void flip_blocks_dxtc5(DXTColBlock* line, unsigned int numBlocks) {
        DXTColBlock* curblock = line;
        DXT5AlphaBlock* alphablock;

        for (unsigned int i = 0; i < numBlocks; i++) {
            alphablock = (DXT5AlphaBlock*)curblock;

            flip_dxt5_alpha(alphablock);

            curblock++;

            std::swap(curblock->row[0], curblock->row[3]);
            std::swap(curblock->row[1], curblock->row[2]);

            curblock++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// CDDSImage public functions

///////////////////////////////////////////////////////////////////////////////
// default constructor
CDDSImage::CDDSImage() :
    m_format(0), m_components(0), m_type(TextureNone), m_valid(false) {
}

CDDSImage::~CDDSImage() {
}

void CDDSImage::create_textureFlat(unsigned int format, unsigned int components, const CTexture& baseImage) {
    assert(format != 0);
    assert(components != 0);
    assert(baseImage.get_depth() == 1);

    // remove any existing images
    clear();

    m_format = format;
    m_components = components;
    m_type = TextureFlat;

    m_images.push_back(baseImage);

    m_valid = true;
}

void CDDSImage::create_texture3D(unsigned int format, unsigned int components, const CTexture& baseImage) {
    assert(format != 0);
    assert(components != 0);
    assert(baseImage.get_depth() > 1);

    // remove any existing images
    clear();

    m_format = format;
    m_components = components;
    m_type = Texture3D;

    m_images.push_back(baseImage);

    m_valid = true;
}

inline bool same_size(const CTexture& a, const CTexture& b) {
    if (a.get_width() != b.get_width())
        return false;
    if (a.get_height() != b.get_height())
        return false;
    if (a.get_depth() != b.get_depth())
        return false;

    return true;
}

void CDDSImage::create_textureCubemap(unsigned int format, unsigned int components, const CTexture& positiveX, const CTexture& negativeX,
    const CTexture& positiveY, const CTexture& negativeY, const CTexture& positiveZ, const CTexture& negativeZ) {
    assert(format != 0);
    assert(components != 0);
    assert(positiveX.get_depth() == 1);

    // verify that all dimensions are the same
    assert(same_size(positiveX, negativeX));
    assert(same_size(positiveX, positiveY));
    assert(same_size(positiveX, negativeY));
    assert(same_size(positiveX, positiveZ));
    assert(same_size(positiveX, negativeZ));

    // remove any existing images
    clear();

    m_format = format;
    m_components = components;
    m_type = TextureCubemap;

    m_images.push_back(positiveX);
    m_images.push_back(negativeX);
    m_images.push_back(positiveY);
    m_images.push_back(negativeY);
    m_images.push_back(positiveZ);
    m_images.push_back(negativeZ);

    m_valid = true;
}

///////////////////////////////////////////////////////////////////////////////
// loads DDS image
//
// filename - fully qualified name of DDS image
// flipImage - specifies whether image is flipped on load, default is true
void CDDSImage::load(const string& filename, bool flipImage) {
    assert(!filename.empty());

    std::ifstream fs(filename.c_str(), ios::binary);
    if (!fs.is_open())
        throw std::exception("ERROR! Could not load file.");
    load(fs, flipImage);
}


///////////////////////////////////////////////////////////////////////////////
// loads DDS image
//
// is - istream to read the image from
// flipImage - specifies whether image is flipped on load, default is true
void CDDSImage::load(istream& is, bool flipImage) {
    // clear any previously loaded images
    clear();

    // Get total bytes in file.
    size_t totalFileSize = is.tellg();
    is.seekg(0, std::ios::end);
    totalFileSize = size_t(is.tellg()) - totalFileSize;
    is.seekg(0, std::ios::beg);

    // read in file marker, make sure its a DDS file
    char filecode[4];
    is.read(filecode, 4);
    if (strncmp(filecode, "DDS ", 4) != 0) {
        throw runtime_error("not a DDS file");
    }

    // read in DDS header
    DDS_HEADER ddsh;
    is.read((char*)&ddsh, sizeof(DDS_HEADER));

    // default to flat texture type (1D, 2D, or rectangle)
    m_type = TextureFlat;

    // check if image is a cubemap
    if (ddsh.dwCaps2 & DDSF_CUBEMAP)
        m_type = TextureCubemap;

    // check if image is a volume texture
    if ((ddsh.dwCaps2 & DDSF_VOLUME) && (ddsh.dwDepth > 0))
        m_type = Texture3D;

    // If file is a DX10 texture, try to determine its format.
    bool isDX10 = (ddsh.ddspf.dwFlags & DDSF_FOURCC) && ddsh.ddspf.dwFourCC == FOURCC_DX10;
    DXGI_FORMAT dxgi_fmt = DXGI_FORMAT(0);
    DDS_HEADER_DXT10 d3d10ext;
    size_t channels = 0;

    size_t bitSize = 0;
    unsigned char* bitData = nullptr;

    if (isDX10)
    {
        is.read((char*)&d3d10ext, sizeof(DDS_HEADER_DXT10));

        // Find DX10 type.
        dxgi_fmt = d3d10ext.dxgiFormat == 0 ? GetDXGIFormat(ddsh.ddspf) : d3d10ext.dxgiFormat;

        // Calculate bits per pixel, channels, etc as necessary.
        ddsh.ddspf.dwRGBBitCount = BitsPerPixel(dxgi_fmt);
        channels += ddsh.ddspf.dwRBitMask > 0 ? 1 : 0;
        channels += ddsh.ddspf.dwGBitMask > 0 ? 1 : 0;
        channels += ddsh.ddspf.dwBBitMask > 0 ? 1 : 0;
        channels += ddsh.ddspf.dwABitMask > 0 ? 1 : 0;
        ddsh.ddspf.dwRGBBitCount *= channels;
        m_components = ddsh.ddspf.dwRGBBitCount / 8;
    }

    // Get the OpenGL format from the fourcc if possible.
    if (isDX10 || ddsh.ddspf.dwFlags & DDSF_FOURCC) {
        switch (ddsh.ddspf.dwFourCC) {
        case FOURCC_ATI1:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            m_components = 4;
            break;
        case FOURCC_BC3U:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            m_components = 4;
            break;
        case FOURCC_ATI2:
            m_format = GL_COMPRESSED_RG_RGTC2;
            m_components = 2;
            break;
        case FOURCC_BC4U:
            m_format = GL_COMPRESSED_RED_RGTC1;
            m_components = 1;
            break;
        case FOURCC_BC4S:
            m_format = GL_COMPRESSED_SIGNED_RED_RGTC1;
            m_components = 1;
            break;
        case FOURCC_BC5U:
            m_format = GL_COMPRESSED_RG_RGTC2;
            m_components = 2;
            break;
        case FOURCC_BC5S:
            m_format = GL_COMPRESSED_SIGNED_RG_RGTC2;
            m_components = 2;
            break;
        case FOURCC_DXT1:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            m_components = 3;
            break;
        case FOURCC_DXT2:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            m_components = 4;
            break;
        case FOURCC_DXT3:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            m_components = 4;
            break;
        case FOURCC_DXT4:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            m_components = 4;
            break;
        case FOURCC_DXT5:
            m_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            m_components = 4;
            break;
        case FOURCC_DX10:// Arbitrary DX10 loading, formatting is determined from the other values in the header
            m_format = DX10formatToGL(dxgi_fmt);
            break;
        case FOURCC_RGBG:
            throw runtime_error("Unable to process RGBG texture format");
        case FOURCC_GRGB:
            throw runtime_error("Unable to process GRGB texture format");
        default:
            throw runtime_error("unknown texture compression '" + fourcc(ddsh.ddspf.dwFourCC) + "'");
        }
    }
    else if (ddsh.ddspf.dwRGBBitCount == 32 &&
        ddsh.ddspf.dwRBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x000000FF &&
        ddsh.ddspf.dwABitMask == 0xFF000000) {
        m_format = GL_BGRA_EXT;
        m_components = 4;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 32 &&
        ddsh.ddspf.dwRBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x000000FF &&
        ddsh.ddspf.dwABitMask == 0x00000000) {
        m_format = GL_BGRA_EXT;
        m_components = 4;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 32 &&
        ddsh.ddspf.dwRBitMask == 0x000000FF &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwABitMask == 0x00000000) {
        m_format = GL_RGBA;
        m_components = 4;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 32 &&
        ddsh.ddspf.dwRBitMask == 0x000000FF &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwABitMask == 0xFF000000) {
        m_format = GL_RGBA;
        m_components = 4;
    }
    else if (
        ddsh.ddspf.dwRGBBitCount == 24 &&
        ddsh.ddspf.dwRBitMask == 0x000000FF &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x00FF0000) {
        m_format = GL_RGB;
        m_components = 3;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 16 &&
        ddsh.ddspf.dwRBitMask == 0x000000FF &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x000000) {
        m_format = GL_RG;
        m_components = 2;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 16 &&
        ddsh.ddspf.dwRBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x000000) {
        m_format = GL_RG;
        m_components = 2;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 8 &&
        ddsh.ddspf.dwRBitMask == 0x000000FF &&
        ddsh.ddspf.dwGBitMask == 0x00000000 &&
        ddsh.ddspf.dwBBitMask == 0x000000) {
        m_format = GL_RED;
        m_components = 1;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 24 &&
        ddsh.ddspf.dwRBitMask == 0x00FF0000 &&
        ddsh.ddspf.dwGBitMask == 0x0000FF00 &&
        ddsh.ddspf.dwBBitMask == 0x000000FF) {
        m_format = GL_BGR_EXT;
        m_components = 3;
    }
    else if (ddsh.ddspf.dwRGBBitCount == 8) {
        m_format = GL_LUMINANCE;
        m_components = 1;
    }
    else {
        throw runtime_error("Unknown texture format for non-DX10 DDS file.");
    }

    // store primary surface width/height/depth
    unsigned int width, height, depth;
    width = ddsh.dwWidth;
    height = ddsh.dwHeight;
    depth = clamp_size(ddsh.dwDepth);   // set to 1 if 0

    // use correct size calculation function depending on whether image is compressed
    unsigned int (CDDSImage:: * sizefunc)(unsigned int, unsigned int);
    sizefunc = (is_compressed() ? &CDDSImage::size_dxtc : &CDDSImage::size_rgb);


    // Load DX10 DDS file (more complex).
    if (isDX10) {
        uint32_t resDim = 0;
        size_t arraySize = 1;
        DXGI_FORMAT format = dxgi_fmt;
        bool isCubeMap = false;
        size_t mipCount = ddsh.dwMipMapCount;
        if (0 == mipCount)
        {
            mipCount = 1;
        }
        if (isDX10)
        {
            arraySize = d3d10ext.arraySize;
            if (arraySize == 0)
            {
                throw runtime_error("array size is 0");
            }

            if (BitsPerPixel(d3d10ext.dxgiFormat) == 0)
            {
                throw runtime_error("pixel size is 0");
            }

            format = d3d10ext.dxgiFormat;

            switch (d3d10ext.resourceDimension)
            {
            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                // D3DX writes 1D textures with a fixed Height of 1.
                if ((ddsh.dwFlags & DDSF_HEIGHT) && height != 1)
                {
                    throw runtime_error("1D texture has an invalid height");
                }
                height = depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
                if (d3d10ext.miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE)
                {
                    arraySize *= 6;
                    isCubeMap = true;
                }
                depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
                if (!(ddsh.dwFlags & DDSF_VOLUME))
                {
                    throw runtime_error("1D texture has an invalid height");
                }

                if (arraySize > 1)
                {
                    throw runtime_error("Volume texture invalid array size");
                }
                break;

            default:
                throw runtime_error("Generic DX10 header exception");
            }

            resDim = d3d10ext.resourceDimension;
        }

        switch (resDim)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if ((arraySize > D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
                (width > D3D11_REQ_TEXTURE1D_U_DIMENSION))
            {
                throw runtime_error("array size count error");
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (isCubeMap)
            {
                // This is the right bound because we set arraySize to (NumCubes*6) above.
                if ((arraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
                    (width > D3D11_REQ_TEXTURECUBE_DIMENSION) ||
                    (height > D3D11_REQ_TEXTURECUBE_DIMENSION))
                {
                    throw runtime_error("cube map face count error");
                }
            }
            else if ((arraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
                (width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
                (height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION))
            {
                throw runtime_error("2D texture size error");
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            if ((arraySize > 1) ||
                (width > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
                (height > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
                (depth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
            {
                throw runtime_error("volume texture size error");
            }
            break;
        }


        size_t bitSize = totalFileSize - is.tellg();
        unsigned char* bitData = new unsigned char[bitSize];
        is.read((char*)bitData, bitSize);
        unsigned char* initData = new unsigned char[bitSize];
        size_t maxsize = bitSize;

        size_t skipMip = 0;
        size_t twidth = 0;
        size_t theight = 0;
        size_t tdepth = 0;

        size_t NumBytes = 0;
        size_t RowBytes = 0;
        size_t NumRows = 0;
        unsigned char* pSrcBits = &bitData[0];
        unsigned char* pEndBits = &bitData[bitSize];

        // Get all array images.
        for (size_t j = 0; j < arraySize; j++)
        {
            size_t w = width;
            size_t h = height;
            size_t d = depth;

            GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, &NumRows);

            // add texture object to array.
            m_images.push_back(CTexture());
            CTexture& img = m_images[j];
            uint8_t* pixels = new uint8_t[NumBytes];
            memcpy(pixels, pSrcBits, NumBytes);
            img.create(width, height, depth, NumBytes, pixels);
            delete[] pixels;


            // NOTE: Ignore mipmaps.
        }
    }

    // ---------------------------------------------------------------------
    // Load non-DX10 DDS file.
    else {
        // load all surfaces for the image (6 surfaces for cubemaps)
        for (unsigned int n = 0; n < (unsigned int)(m_type == TextureCubemap ? 6 : 1); n++) {
            // add empty texture object
            m_images.push_back(CTexture());

            // get reference to newly added texture object
            CTexture& img = m_images[n];
            unsigned int size = (this->*sizefunc)(width, height) * depth;
            uint8_t* pixels = new uint8_t[size];
            is.read((char*)pixels, size);
            img.create(width, height, depth, size, pixels);
            delete[] pixels;

            if (flipImage)
                flip(img);

            unsigned int w = clamp_size(width >> 1);
            unsigned int h = clamp_size(height >> 1);
            unsigned int d = clamp_size(depth >> 1);
            size_t NumBytes = 0;
            size_t RowBytes = 0;
            size_t NumRows = 0;

            // NOTE: ignore all mipmaps.
        }
    }


    // swap cubemaps on y axis (since image is flipped in OGL)
    if (m_type == TextureCubemap && flipImage) {
        CTexture tmp;
        tmp = m_images[3];
        m_images[3] = m_images[2];
        m_images[2] = tmp;
    }

    m_valid = true;
}

void CDDSImage::write_texture(const CTexture& texture, ostream& os) {
    assert(get_num_mipmaps() == texture.get_num_mipmaps());

    os.write((char*)(uint8_t*)texture, texture.get_size());

    for (unsigned int i = 0; i < texture.get_num_mipmaps(); i++) {
        const CSurface& mipmap = texture.get_mipmap(i);
        os.write((char*)(uint8_t*)mipmap, mipmap.get_size());
    }
}

void CDDSImage::save(const std::string& filename, bool flipImage) {
    assert(m_valid);
    assert(m_type != TextureNone);

    DDS_HEADER ddsh;
    unsigned int headerSize = sizeof(DDS_HEADER);
    memset(&ddsh, 0, headerSize);
    ddsh.dwSize = headerSize;
    ddsh.dwFlags = DDSF_CAPS | DDSF_WIDTH | DDSF_HEIGHT | DDSF_PIXELFORMAT;
    ddsh.dwHeight = get_height();
    ddsh.dwWidth = get_width();

    if (is_compressed()) {
        ddsh.dwFlags |= DDSF_LINEARSIZE;
        ddsh.dwPitchOrLinearSize = get_size();
    }
    else {
        ddsh.dwFlags |= DDSF_PITCH;
        ddsh.dwPitchOrLinearSize = get_dword_aligned_linesize(get_width(), m_components * 8);
    }

    if (m_type == Texture3D) {
        ddsh.dwFlags |= DDSF_DEPTH;
        ddsh.dwDepth = get_depth();
    }

    if (get_num_mipmaps() > 0) {
        ddsh.dwFlags |= DDSF_MIPMAPCOUNT;
        ddsh.dwMipMapCount = get_num_mipmaps() + 1;
    }

    ddsh.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);

    if (is_compressed()) {
        ddsh.ddspf.dwFlags = DDSF_FOURCC;

        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT1;
        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT3;
        if (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
            ddsh.ddspf.dwFourCC = FOURCC_DXT5;
    }
    else {
        ddsh.ddspf.dwFlags = (m_components == 4) ? DDSF_RGBA : DDSF_RGB;
        ddsh.ddspf.dwRGBBitCount = m_components * 8;
        ddsh.ddspf.dwRBitMask = 0x00ff0000;
        ddsh.ddspf.dwGBitMask = 0x0000ff00;
        ddsh.ddspf.dwBBitMask = 0x000000ff;

        if (m_components == 4) {
            ddsh.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
            ddsh.ddspf.dwABitMask = 0xff000000;
        }
    }

    ddsh.dwCaps1 = DDSF_TEXTURE;

    if (m_type == TextureCubemap) {
        ddsh.dwCaps1 |= DDSF_COMPLEX;
        ddsh.dwCaps2 = DDSF_CUBEMAP | DDSF_CUBEMAP_ALL_FACES;
    }

    if (m_type == Texture3D) {
        ddsh.dwCaps1 |= DDSF_COMPLEX;
        ddsh.dwCaps2 = DDSF_VOLUME;
    }

    if (get_num_mipmaps() > 0)
        ddsh.dwCaps1 |= DDSF_COMPLEX | DDSF_MIPMAP;

    // open file
    ofstream of;
    of.exceptions(ios::failbit);
    of.open(filename.c_str(), ios::binary);

    // write file header
    of.write("DDS ", 4);

    // write dds header
    of.write((char*)&ddsh, sizeof(DDS_HEADER));

    if (m_type != TextureCubemap) {
        CTexture tex = m_images[0];
        if (flipImage)
            flip_texture(tex);
        write_texture(tex, of);
    }
    else {
        assert(m_images.size() == 6);

        for (unsigned int i = 0; i < m_images.size(); i++) {
            CTexture cubeFace;

            if (i == 2)
                cubeFace = m_images[3];
            else if (i == 3)
                cubeFace = m_images[2];
            else
                cubeFace = m_images[i];

            if (flipImage)
                flip_texture(cubeFace);
            write_texture(cubeFace, of);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// free image memory
void CDDSImage::clear() {
    m_components = 0;
    m_format = 0;
    m_type = TextureNone;
    m_valid = false;

    m_images.clear();
}

#ifndef NV_DDS_NO_GL_SUPPORT
#if !defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0)
///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 1D texture
void CDDSImage::upload_texture1D() {
    assert(m_valid);
    assert(!m_images.empty());

    const CTexture& baseImage = m_images[0];

    assert(baseImage.get_height() == 1);
    assert(baseImage.get_width() > 0);

    if (is_compressed()) {
        glCompressedTexImage1D(GL_TEXTURE_1D, 0, m_format, baseImage.get_width(), 0, baseImage.get_size(), baseImage);

        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++) {
            const CSurface& mipmap = baseImage.get_mipmap(i);
            glCompressedTexImage1D(GL_TEXTURE_1D, i + 1, m_format, mipmap.get_width(), 0, mipmap.get_size(), mipmap);
        }
    }
    else {
        GLint alignment = -1;
        if (!is_dword_aligned()) {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage1D(GL_TEXTURE_1D, 0, m_components, baseImage.get_width(), 0, m_format, GL_UNSIGNED_BYTE, baseImage);

        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++) {
            const CSurface& mipmap = baseImage.get_mipmap(i);

            glTexImage1D(GL_TEXTURE_1D, i + 1, m_components, mipmap.get_width(), 0, m_format, GL_UNSIGNED_BYTE, mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 2D texture
//
// imageIndex - allows you to optionally specify other loaded surfaces for 2D
//              textures such as a face in a cubemap or a slice in a volume
//
//              default: 0
//
// target     - allows you to optionally specify a different texture target for
//              the 2D texture such as a specific face of a cubemap
//
//              default: GL_TEXTURE_2D
void CDDSImage::upload_texture2D(uint32_t imageIndex, uint32_t target) {
    assert(m_valid);
    assert(!m_images.empty());
    assert(imageIndex < m_images.size());
    assert(m_images[imageIndex]);

    const CTexture& image = m_images[imageIndex];

    assert(image.get_height() > 0);
    assert(image.get_width() > 0);
    assert(
        target == GL_TEXTURE_2D || (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));

    if (is_compressed()) {
        glCompressedTexImage2D(target, 0, m_format, image.get_width(), image.get_height(), 0, image.get_size(), image);

        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++) {
            const CSurface& mipmap = image.get_mipmap(i);

            glCompressedTexImage2D(target, i + 1, m_format, mipmap.get_width(), mipmap.get_height(), 0, mipmap.get_size(), mipmap);
        }
    }
    else {
        GLint alignment = -1;
        if (!is_dword_aligned()) {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage2D(target, 0, m_components, image.get_width(), image.get_height(), 0, m_format, GL_UNSIGNED_BYTE, image);

        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++) {
            const CSurface& mipmap = image.get_mipmap(i);

            glTexImage2D(target, i + 1, m_components, mipmap.get_width(), mipmap.get_height(), 0, m_format, GL_UNSIGNED_BYTE, mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
}

#ifndef GL_ES_VERSION_2_0
///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 3D texture
void CDDSImage::upload_texture3D() {
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == Texture3D);

    const CTexture& baseImage = m_images[0];

    assert(baseImage.get_depth() >= 1);

    if (is_compressed()) {
        glCompressedTexImage3D(GL_TEXTURE_3D, 0, m_format, baseImage.get_width(), baseImage.get_height(), baseImage.get_depth(), 0, baseImage.get_size(),
            baseImage);

        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++) {
            const CSurface& mipmap = baseImage.get_mipmap(i);

            glCompressedTexImage3D(GL_TEXTURE_3D, i + 1, m_format, mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 0, mipmap.get_size(),
                mipmap);
        }
    }
    else {
        GLint alignment = -1;
        if (!is_dword_aligned()) {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage3D(GL_TEXTURE_3D, 0, m_components, baseImage.get_width(), baseImage.get_height(), baseImage.get_depth(), 0, m_format, GL_UNSIGNED_BYTE,
            baseImage);

        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++) {
            const CSurface& mipmap = baseImage.get_mipmap(i);

            glTexImage3D(GL_TEXTURE_3D, i + 1, m_components, mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 0, m_format, GL_UNSIGNED_BYTE,
                mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed cubemap texture
void CDDSImage::upload_textureCubemap() {
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == TextureCubemap);
    assert(m_images.size() == 6);

    GLenum target;

    // loop through cubemap faces and load them as 2D textures
    for (unsigned int n = 0; n < 6; n++) {
        // specify cubemap face
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + n;
        upload_texture2D(n, target);
    }
}
#endif

bool CDDSImage::is_compressed() {
    return (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
        || (m_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
        || (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
        || (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
        || (m_format == GL_COMPRESSED_RED_RGTC1)
        || (m_format == GL_COMPRESSED_SIGNED_RED_RGTC1)
        || (m_format == GL_COMPRESSED_RG_RGTC2)
        || (m_format == GL_COMPRESSED_SIGNED_RG_RGTC2)
        ;
}

///////////////////////////////////////////////////////////////////////////////
// clamps input size to [1-size]
inline unsigned int CDDSImage::clamp_size(unsigned int size) {
    if (size <= 0)
        size = 1;

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// CDDSImage private functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// calculates size of DXTC texture in bytes
inline unsigned int CDDSImage::size_dxtc(unsigned int width, unsigned int height) {
    return ((width + 3) / 4) * ((height + 3) / 4) * (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
}

///////////////////////////////////////////////////////////////////////////////
// calculates size of uncompressed RGB texture in bytes
inline unsigned int CDDSImage::size_rgb(unsigned int width, unsigned int height) {
    return width * height * m_components;
}

///////////////////////////////////////////////////////////////////////////////
// flip image around X axis
void CDDSImage::flip(CSurface& surface) {
    unsigned int linesize;
    unsigned int offset;

    if (!is_compressed()) {
        assert(surface.get_depth() > 0);

        unsigned int imagesize = surface.get_size() / surface.get_depth();
        linesize = imagesize / surface.get_height();

        uint8_t* tmp = new uint8_t[linesize];

        for (unsigned int n = 0; n < surface.get_depth(); n++) {
            offset = imagesize * n;
            uint8_t* top = (uint8_t*)surface + offset;
            uint8_t* bottom = top + (imagesize - linesize);

            for (unsigned int i = 0; i < (surface.get_height() >> 1); i++) {
                // swap
                memcpy(tmp, bottom, linesize);
                memcpy(bottom, top, linesize);
                memcpy(top, tmp, linesize);

                top += linesize;
                bottom -= linesize;
            }
        }

        delete[] tmp;
    }
    else {
        void (*flipblocks)(DXTColBlock*, unsigned int);
        unsigned int xblocks = surface.get_width() / 4;
        unsigned int yblocks = surface.get_height() / 4;
        unsigned int blocksize;

        switch (m_format) {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            blocksize = 8;
            flipblocks = flip_blocks_dxtc1;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            blocksize = 16;
            flipblocks = flip_blocks_dxtc3;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            blocksize = 16;
            flipblocks = flip_blocks_dxtc5;
            break;
        default:
            return;
        }

        linesize = xblocks * blocksize;

        DXTColBlock* top;
        DXTColBlock* bottom;

        uint8_t* tmp = new uint8_t[linesize];

        for (unsigned int j = 0; j < (yblocks >> 1); j++) {
            top = (DXTColBlock*)((uint8_t*)surface + j * linesize);
            bottom = (DXTColBlock*)((uint8_t*)surface + (((yblocks - j) - 1) * linesize));

            flipblocks(top, xblocks);
            flipblocks(bottom, xblocks);

            // swap
            memcpy(tmp, bottom, linesize);
            memcpy(bottom, top, linesize);
            memcpy(top, tmp, linesize);
        }

        delete[] tmp;
    }
}

void CDDSImage::flip_texture(CTexture& texture) {
    flip(texture);

    for (unsigned int i = 0; i < texture.get_num_mipmaps(); i++) {
        flip(texture.get_mipmap(i));
    }
}

///////////////////////////////////////////////////////////////////////////////
// CTexture implementation
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// default constructor
CTexture::CTexture() :
    CSurface()  // initialize base class part
{
}

///////////////////////////////////////////////////////////////////////////////
// creates an empty texture
CTexture::CTexture(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t* pixels) :
    CSurface(w, h, d, imgsize, pixels)  // initialize base class part
{
}

CTexture::~CTexture() {
}

///////////////////////////////////////////////////////////////////////////////
// copy constructor
CTexture::CTexture(const CTexture& copy) :
    CSurface(copy) {
    for (unsigned int i = 0; i < copy.get_num_mipmaps(); i++)
        m_mipmaps.push_back(copy.get_mipmap(i));
}

///////////////////////////////////////////////////////////////////////////////
// assignment operator
CTexture& CTexture::operator=(const CTexture& rhs) {
    if (this != &rhs) {
        CSurface::operator =(rhs);

        m_mipmaps.clear();
        for (unsigned int i = 0; i < rhs.get_num_mipmaps(); i++)
            m_mipmaps.push_back(rhs.get_mipmap(i));
    }

    return *this;
}

void CTexture::create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t* pixels) {
    CSurface::create(w, h, d, imgsize, pixels);

    m_mipmaps.clear();
}

void CTexture::clear() {
    CSurface::clear();

    m_mipmaps.clear();
}

///////////////////////////////////////////////////////////////////////////////
// CSurface implementation
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// default constructor
CSurface::CSurface() :
    m_width(0), m_height(0), m_depth(0), m_size(0), m_pixels(NULL) {
}

///////////////////////////////////////////////////////////////////////////////
// creates an empty image
CSurface::CSurface(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t* pixels) :
    m_width(0), m_height(0), m_depth(0), m_size(0), m_pixels(NULL) {
    create(w, h, d, imgsize, pixels);
}

///////////////////////////////////////////////////////////////////////////////
// copy constructor
CSurface::CSurface(const CSurface& copy) :
    m_width(0), m_height(0), m_depth(0), m_size(0), m_pixels(NULL) {
    if (copy.get_size() != 0) {
        m_size = copy.get_size();
        m_width = copy.get_width();
        m_height = copy.get_height();
        m_depth = copy.get_depth();

        m_pixels = new uint8_t[m_size];
        memcpy(m_pixels, copy, m_size);
    }
}

///////////////////////////////////////////////////////////////////////////////
// assignment operator
CSurface& CSurface::operator=(const CSurface& rhs) {
    if (this != &rhs) {
        clear();

        if (rhs.get_size()) {
            m_size = rhs.get_size();
            m_width = rhs.get_width();
            m_height = rhs.get_height();
            m_depth = rhs.get_depth();

            m_pixels = new uint8_t[m_size];
            memcpy(m_pixels, rhs, m_size);
        }
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// clean up image memory
CSurface::~CSurface() {
    clear();
}

///////////////////////////////////////////////////////////////////////////////
// returns a pointer to image
CSurface::operator uint8_t* () const {
    return m_pixels;
}

///////////////////////////////////////////////////////////////////////////////
// creates an empty image
void CSurface::create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t* pixels) {
    assert(w != 0);
    assert(h != 0);
    assert(d != 0);
    assert(imgsize != 0);
    assert(pixels);

    clear();

    m_width = w;
    m_height = h;
    m_depth = d;
    m_size = imgsize;
    m_pixels = new uint8_t[imgsize];
    memcpy(m_pixels, pixels, imgsize);
}

///////////////////////////////////////////////////////////////////////////////
// free surface memory
void CSurface::clear() {
    if (m_pixels != NULL) {
        delete[] m_pixels;
        m_pixels = NULL;
    }
}