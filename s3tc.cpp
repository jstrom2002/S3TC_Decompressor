#include "s3tc.h"

namespace S3TC
{
    // DXT1 (BC1) defines shifting in precalculated alpha
    uint32_t __DXT1_LUT_COLOR_SHIFT_A = 24;
    // DXT1 (BC1) defines shifting in precalculated [R,G,B] components
    uint32_t __DXT1_LUT_COLOR_SHIFT_R = 16;
    uint32_t __DXT1_LUT_COLOR_SHIFT_G = 8;
    uint32_t __DXT1_LUT_COLOR_SHIFT_B = 0;

    // DXT1 (BC1) pre calculated values for alpha codes
    uint32_t __DXT1_LUT_COLOR_VALUE_A = (uint32_t)(0xff << __DXT1_LUT_COLOR_SHIFT_A);

    // DXT1 (BC1) precalculated [R,G,B] components for all 4 codes's 
    uint32_t* __DXT1_LUT_COLOR_VALUE_R = nullptr;
    uint32_t* __DXT1_LUT_COLOR_VALUE_G = nullptr;
    uint32_t* __DXT1_LUT_COLOR_VALUE_B = nullptr;



    // DXT3 (BC2) defines shifting in precalculated alpha
    uint32_t __DXT3_LUT_COLOR_SHIFT_A = 24;

    // DXT3 (BC2) defines shifting in precalculated [R,G,B] components
    uint32_t __DXT3_LUT_COLOR_SHIFT_R = 16;
    uint32_t __DXT3_LUT_COLOR_SHIFT_G = 8;
    uint32_t __DXT3_LUT_COLOR_SHIFT_B = 0;

    // DXT3 (BC2) pre calculated values for alpha codes
    uint32_t* __DXT3_LUT_COLOR_VALUE_A = nullptr;

    // DXT3 (BC2) precalculated [R,G,B] components for all 4 codes's 
    uint32_t* __DXT3_LUT_COLOR_VALUE_R = nullptr;
    uint32_t* __DXT3_LUT_COLOR_VALUE_G = nullptr;
    uint32_t* __DXT3_LUT_COLOR_VALUE_B = nullptr;



    // DXT5 (BC3) defines shifting in precalculated alpha
    uint32_t __DXT5_LUT_COLOR_SHIFT_A = 24;

    // DXT5 (BC3) defines shifting in precalculated [R,G,B] components
    uint32_t __DXT5_LUT_COLOR_SHIFT_R = 16;
    uint32_t __DXT5_LUT_COLOR_SHIFT_G = 8;
    uint32_t __DXT5_LUT_COLOR_SHIFT_B = 0;

    // DXT5 (BC3) pre calculated values for alpha codes
    uint32_t* __DXT5_LUT_COLOR_VALUE_A = nullptr;

    // DXT5 (BC3) precalculated [R,G,B] components for all 4 codes's 
    uint32_t* __DXT5_LUT_COLOR_VALUE_R = nullptr;
    uint32_t* __DXT5_LUT_COLOR_VALUE_G = nullptr;
    uint32_t* __DXT5_LUT_COLOR_VALUE_B = nullptr;



    void DXT1ReleaseLUTs()
    {
        if (nullptr != __DXT1_LUT_COLOR_VALUE_R)
        {
            delete[] __DXT1_LUT_COLOR_VALUE_R;
            __DXT1_LUT_COLOR_VALUE_R = nullptr;
        }
        if (nullptr != __DXT1_LUT_COLOR_VALUE_G)
        {
            delete[] __DXT1_LUT_COLOR_VALUE_G;
            __DXT1_LUT_COLOR_VALUE_B = nullptr;
        }
        if (nullptr != __DXT1_LUT_COLOR_VALUE_B)
        {
            delete[] __DXT1_LUT_COLOR_VALUE_B;
            __DXT1_LUT_COLOR_VALUE_B = nullptr;
        }
    }


    void __DXT1_LUT_Build()
    {
        // DXT1 (BC1) pre calculated values for r & b codes
        unsigned char __LUT_DXT1_4x8[32] = // 0x00 - 0x1f (0-31)
        {
            0,   8,  16,  25,  33,  41,  49,  58,
           66,  74,  82,  90,  99, 107, 115, 123,
          132, 140, 148, 156, 164, 173, 181, 189,
          197, 205, 214, 222, 230, 238, 247, 255
        };

        // DXT1 (BC1) pre calculated values for g codes
        unsigned char __LUT_DXT1_8x8[64] = // 0x00 - 0x3f (0-63)
        {
            0,   4,   8,  12,  16,  20,  24,  28,
           32,  36,  40,  45,  49,  53,  57,  61,
           65,  69,  73,  77,  81,  85,  89,  93,
           97, 101, 105, 109, 113, 117, 121, 125,
          130, 134, 138, 142, 146, 150, 154, 158,
          162, 166, 170, 174, 178, 182, 186, 190,
          194, 198, 202, 206, 210, 214, 219, 223,
          227, 231, 235, 239, 243, 247, 251, 255
        };

        __DXT1_LUT_COLOR_VALUE_A = (uint32_t)(0xff << __DXT1_LUT_COLOR_SHIFT_A);
        if (nullptr == __DXT1_LUT_COLOR_VALUE_R)
        {
            __DXT1_LUT_COLOR_VALUE_R = new uint32_t[8192];  // 4*2*32*32 (4:code)*(2:alpha)*(32:c0(r)matrix4x8)*(32:c1(r)matrix4x8)
            __DXT1_LUT_COLOR_VALUE_G = new uint32_t[32768]; // 4*2*64*64 (4:code)*(2:alpha)*(64:c0(g)matrix8x8)*(64:c1(g)matrix8x8)
            __DXT1_LUT_COLOR_VALUE_B = new uint32_t[8192];  // 4*2*32*32 (4:code)*(2:alpha)*(32:c0(b)matrix4x8)*(32:c1(b)matrix4x8)
        }

        for (int ac = 0; ac <= 1; ac++)
        {
            for (int cc0 = 0; cc0 < 32; cc0++)
            {
                for (int cc1 = 0; cc1 < 32; cc1++)
                {
                    int index = ((cc0 << 6) | (cc1 << 1) | (ac)) << 2; // 2 bits for ccfn
                    __DXT1_LUT_COLOR_VALUE_R[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_R);
                    __DXT1_LUT_COLOR_VALUE_B[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_B);
                    __DXT1_LUT_COLOR_VALUE_R[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_R);
                    __DXT1_LUT_COLOR_VALUE_B[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_B);
                    if (cc0 > cc1)
                    {
                        // p2 = ((2*c0)+(c1))/3
                        __DXT1_LUT_COLOR_VALUE_R[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0] * 2) + (__LUT_DXT1_4x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
                        __DXT1_LUT_COLOR_VALUE_B[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0] * 2) + (__LUT_DXT1_4x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
                        // p3 = ((c0)+(2*c1))/3              
                        __DXT1_LUT_COLOR_VALUE_R[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
                        __DXT1_LUT_COLOR_VALUE_B[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
                    }
                    else // c0 <= c1
                    {
                        // p2 = (c0/2)+(c1/2)
                        __DXT1_LUT_COLOR_VALUE_R[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0] / 2) + (__LUT_DXT1_4x8[cc1] / 2)))) << __DXT1_LUT_COLOR_SHIFT_R);
                        __DXT1_LUT_COLOR_VALUE_B[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0] / 2) + (__LUT_DXT1_4x8[cc1] / 2)))) << __DXT1_LUT_COLOR_SHIFT_B);
                        if (ac == 0)
                        {
                            // p3 = (color0 + 2*color1) / 3
                            __DXT1_LUT_COLOR_VALUE_R[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
                            __DXT1_LUT_COLOR_VALUE_B[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
                        }
                        else // tr == 1
                        {
                            // p3 == 0
                            __DXT1_LUT_COLOR_VALUE_R[index | 3] = 0; // transparent black
                            __DXT1_LUT_COLOR_VALUE_B[index | 3] = 0; // transparent black
                        }
                    }
                }//cc1
            }//cc0
        }//ac
        for (int ac = 0; ac <= 1; ac++)
        {
            for (int cc0 = 0; cc0 < 64; cc0++)
            {
                for (int cc1 = 0; cc1 < 64; cc1++)
                {
                    int index = ((cc0 << 7) | (cc1 << 1) | (ac)) << 2; // 2 bits for ccfn
                    __DXT1_LUT_COLOR_VALUE_G[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_8x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_G);
                    __DXT1_LUT_COLOR_VALUE_G[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_8x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_G);
                    if (cc0 > cc1)
                    {
                        // p2 = ((2*c0)+(c1))/3
                        __DXT1_LUT_COLOR_VALUE_G[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_8x8[cc0] * 2) + (__LUT_DXT1_8x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_G);
                        // p3 = ((c0)+(2*c1))/3
                        __DXT1_LUT_COLOR_VALUE_G[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_8x8[cc0]) + (__LUT_DXT1_8x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_G);
                    }
                    else // c0 <= c1
                    {
                        // p2 = (c0/2)+(c1/2)
                        __DXT1_LUT_COLOR_VALUE_G[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_8x8[cc0] / 2) + (__LUT_DXT1_8x8[cc1]) / 2))) << __DXT1_LUT_COLOR_SHIFT_G);
                        if (ac == 0)
                        {
                            // p3 = (color0 + 2*color1) / 3
                            __DXT1_LUT_COLOR_VALUE_G[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((unsigned char)(((__LUT_DXT1_8x8[cc0] / 2) + (__LUT_DXT1_8x8[cc1]) / 2))) << __DXT1_LUT_COLOR_SHIFT_G);
                        }
                        else
                        {
                            // p3 == 0
                            __DXT1_LUT_COLOR_VALUE_G[index | 3] = 0; // transparent black
                        }
                    }
                }//cc1
            }//cc0
        }//ac
    }



    void S3TC_Decompressor::DXT1Decompress(const unsigned char* p_input)
    {
        if (NULL == __DXT1_LUT_COLOR_VALUE_R)
        {
            __DXT1_LUT_Build();
        }

        // direct copy paste from c# code, not even comments changed

        unsigned char* source = (unsigned char*)p_input; // block size: 64bit
        uint32_t* target = (uint32_t*)image;
        uint32_t target_4scans = (width << 2);
        uint32_t x_block_count = (width + 3) / 4;
        uint32_t y_block_count = (height + 3) / 4;

        if (x_block_count * 4 != width || y_block_count * 4 != height)
        {
            // for images that do not fit in 4x4 texel bounds
            goto ProcessWithCheckingTexelBounds;
        }

        // NOTICE: source and target ARE aligned as 4x4 texels

        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            // texel: advance by 4 texels
            uint32_t* texel_x = target;
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 8, texel_x += 4)
            {
                // read DXT1 (BC1) block data
                uint16_t cc0 = *(uint16_t*)(source);      // 00-01 : cc0       (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 2);  // 02-03 : cc1       (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 4);  // 04-07 : ccfn LUT  (32bits) 4x4x2bits
                uint32_t ac = (uint32_t)(cc0 > cc1 ? 0 : 4);

                // color code [r,g,b] indexes to luts 
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 3) | (uint32_t)((cc1 & 0xf800) >> 8)) | ac;
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 4) | (uint32_t)((cc1 & 0x07E0) >> 2)) | ac;
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 8) | (uint32_t)((cc1 & 0x001F) << 3)) | ac;

                // process 4x4 texels
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width) // next line
                {
                    for (int bx = 0; bx < 4; bx++, ccfnlut >>= 2)
                    {
                        uint32_t ccfn = (ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT1_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT1_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT1_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx        
                }//by
            }//x_block
        }//y_block
        return;
    ProcessWithCheckingTexelBounds:
        //
        // NOTICE: source and target ARE NOT aligned to 4x4 texels, 
        //         We must check for End Of Image (EOI) in this case.
        //
        // lazy to write boundary separate processings.
        // Just end of image (EOI) pointer check only.
        // considering that I haven't encountered any image that is not
        // aligned to 4x4 texel this almost never should be called.
        // and takes 0.5~1 ms more time processing 2MB pixel images.
        //
        uint32_t* EOI = target + (width * height);
        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            uint32_t* texel_x = target;
            // texel: advance by 4 texels
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 8, texel_x += 4)
            {
                // read DXT1 (BC1) block data
                uint16_t cc0 = *(uint16_t*)(source);      // 00-01 : cc0       (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 2);  // 02-03 : cc1       (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 4);  // 04-07 : ccfn LUT  (32bits) 4x4x2bits
                uint32_t ac = (uint32_t)(cc0 > cc1 ? 0 : 4);

                // color code [r,g,b] indexes to lut(s)
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 3) | (uint32_t)((cc1 & 0xf800) >> 8)) | ac;
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 4) | (uint32_t)((cc1 & 0x07E0) >> 2)) | ac;
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 8) | (uint32_t)((cc1 & 0x001F) << 3)) | ac;

                // process 4x4 texels
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width) // next line
                {
                    //############################################################
                    // Check Y Bound (break: no more texels available for block)
                    if (texel >= EOI) break;
                    //############################################################
                    for (int bx = 0; bx < 4; bx++, ccfnlut >>= 2)
                    {
                        //############################################################
                        // Check X Bound (continue: need ccfnlut to complete shift)
                        if (texel + bx >= EOI) continue;
                        //############################################################
                        uint32_t ccfn = (ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT1_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT1_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT1_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx        
                }//by
            }//x_block
        }//y_block
        DXT1ReleaseLUTs();
    }






    void DXT3ReleaseLUTs()
    {
        if (nullptr != __DXT3_LUT_COLOR_VALUE_A)
        {
            delete[] __DXT3_LUT_COLOR_VALUE_A;
            __DXT3_LUT_COLOR_VALUE_A = nullptr;
        }
        if (nullptr != __DXT3_LUT_COLOR_VALUE_R)
        {
            delete[] __DXT3_LUT_COLOR_VALUE_R;
            __DXT3_LUT_COLOR_VALUE_R = nullptr;
        }
        if (nullptr != __DXT3_LUT_COLOR_VALUE_G)
        {
            delete[] __DXT3_LUT_COLOR_VALUE_G;
            __DXT3_LUT_COLOR_VALUE_B = nullptr;
        }
        if (nullptr != __DXT3_LUT_COLOR_VALUE_B)
        {
            delete[] __DXT3_LUT_COLOR_VALUE_B;
            __DXT3_LUT_COLOR_VALUE_B = nullptr;
        }
    }

    // builds static __DXT3_LUT_COLOR_VALUE_A[] look-up table
    void __DXT3_LUT_COLOR_VALUE_A_Build()
    {
        __DXT3_LUT_COLOR_VALUE_A = new uint32_t[64]; // 8 * 256 * 256

        // DXT3 (BC2) pre calculated values for a codes
        unsigned char __DXT3_LUT_8x8[16] = // 0x00 - 0x0f (0-15)
        {
            0,  17,  34,  51,  68,  85, 102, 119,
          136, 153, 170, 187, 204, 221, 238, 255
        };

        for (int i = 0; i <= 15; i++)
        {
            __DXT3_LUT_COLOR_VALUE_A[i] = __DXT3_LUT_8x8[i] << __DXT3_LUT_COLOR_SHIFT_A;
        }
    }

    // builds static __DXT3_LUT_COLOR_VALUE_RGB[R,G,B] look-up table's
    void __DXT3_LUT_COLOR_VALUE_RGB_Build()
    {
        // DXT3 (BC2) pre calculated values for r & b codes
        unsigned char __DXT3_LUT_4x8[32] = // 0x00 - 0x1f (0-31)
        {
            0,   8,  16,  25,  33,  41,  49,  58,
           66,  74,  82,  90,  99, 107, 115, 123,
          132, 140, 148, 156, 164, 173, 181, 189,
          197, 205, 214, 222, 230, 238, 247, 255
        };

        // DXT3 (BC2) pre calculated values for g codes
        unsigned char __DXT3_LUT_8x8[64] = // 0x00 - 0x3f (0-63)
        {
            0,   4,   8,  12,  16,  20,  24,  28,
           32,  36,  40,  45,  49,  53,  57,  61,
           65,  69,  73,  77,  81,  85,  89,  93,
           97, 101, 105, 109, 113, 117, 121, 125,
          130, 134, 138, 142, 146, 150, 154, 158,
          162, 166, 170, 174, 178, 182, 186, 190,
          194, 198, 202, 206, 210, 214, 219, 223,
          227, 231, 235, 239, 243, 247, 251, 255
        };

        __DXT3_LUT_COLOR_VALUE_R = new uint32_t[4096];  // 4*32*32
        __DXT3_LUT_COLOR_VALUE_G = new uint32_t[16384]; // 4*64*64
        __DXT3_LUT_COLOR_VALUE_B = new uint32_t[4096];  // 4*32*32

        for (int cc0 = 0; cc0 < 32; cc0++)
        {
            for (int cc1 = 0; cc1 < 32; cc1++)
            {
                int index = ((cc0 << 5) | cc1) << 2;
                __DXT3_LUT_COLOR_VALUE_R[index | 0] = (uint32_t)(((uint32_t)__DXT3_LUT_4x8[cc0]) << __DXT3_LUT_COLOR_SHIFT_R);
                __DXT3_LUT_COLOR_VALUE_B[index | 0] = (uint32_t)(((uint32_t)__DXT3_LUT_4x8[cc0]) << __DXT3_LUT_COLOR_SHIFT_B);
                __DXT3_LUT_COLOR_VALUE_R[index | 1] = (uint32_t)(((uint32_t)__DXT3_LUT_4x8[cc1]) << __DXT3_LUT_COLOR_SHIFT_R);
                __DXT3_LUT_COLOR_VALUE_B[index | 1] = (uint32_t)(((uint32_t)__DXT3_LUT_4x8[cc1]) << __DXT3_LUT_COLOR_SHIFT_B);
                // Each RGB image data block is encoded according to the BC1 formats, 
                // with the exception that the two code bits always use the non-transparent encodings. 
                // In other words, they are treated as though color0 > color1, 
                // regardless of the actual values of color0 and color1.   
                // p2 = ((2*c0)+(c1))/3
                __DXT3_LUT_COLOR_VALUE_R[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_4x8[cc0] * 2) + (__DXT3_LUT_4x8[cc1])) / 3)) << __DXT3_LUT_COLOR_SHIFT_R);
                __DXT3_LUT_COLOR_VALUE_B[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_4x8[cc0] * 2) + (__DXT3_LUT_4x8[cc1])) / 3)) << __DXT3_LUT_COLOR_SHIFT_B);
                // p3 = ((c0)+(2*c1))/3
                __DXT3_LUT_COLOR_VALUE_R[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_4x8[cc0]) + (__DXT3_LUT_4x8[cc1] * 2)) / 3)) << __DXT3_LUT_COLOR_SHIFT_R);
                __DXT3_LUT_COLOR_VALUE_B[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_4x8[cc0]) + (__DXT3_LUT_4x8[cc1] * 2)) / 3)) << __DXT3_LUT_COLOR_SHIFT_B);
            }
        }
        for (int cc0 = 0; cc0 < 64; cc0++)
        {
            for (int cc1 = 0; cc1 < 64; cc1++)
            {
                int index = ((cc0 << 6) | cc1) << 2;
                __DXT3_LUT_COLOR_VALUE_G[index | 0] = (uint32_t)(((uint32_t)__DXT3_LUT_8x8[cc0]) << __DXT3_LUT_COLOR_SHIFT_G);
                __DXT3_LUT_COLOR_VALUE_G[index | 1] = (uint32_t)(((uint32_t)__DXT3_LUT_8x8[cc1]) << __DXT3_LUT_COLOR_SHIFT_G);
                // Each RGB image data block is encoded according to the BC1 formats, 
                // with the exception that the two code bits always use the non-transparent encodings. 
                // In other words, they are treated as though color0 > color1, 
                // regardless of the actual values of color0 and color1.      
                // p2 = ((2*c0)+(c1))/3
                __DXT3_LUT_COLOR_VALUE_G[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_8x8[cc0] * 2) + (__DXT3_LUT_8x8[cc1])) / 3)) << __DXT3_LUT_COLOR_SHIFT_G);
                // p3 = ((c0)+(2*c1))/3
                __DXT3_LUT_COLOR_VALUE_G[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT3_LUT_8x8[cc0]) + (__DXT3_LUT_8x8[cc1] * 2)) / 3)) << __DXT3_LUT_COLOR_SHIFT_G);
            }
        }
    }

    void S3TC_Decompressor::DXT3Decompress(const unsigned char* p_input)
    {
        if (NULL == __DXT3_LUT_COLOR_VALUE_A)
        {
            __DXT3_LUT_COLOR_VALUE_A_Build();
            __DXT3_LUT_COLOR_VALUE_RGB_Build();
        }

        // direct copy paste from c# code, not even comments changed

        unsigned char* source = (unsigned char*)p_input;
        uint32_t* target = (uint32_t*)image;
        uint32_t target_4scans = (width << 2);
        uint32_t x_block_count = (width + 3) >> 2;
        uint32_t y_block_count = (height + 3) >> 2;

        //############################################################
        if ((x_block_count << 2) != width || (y_block_count << 2) != height)
        {
            // for images that do not fit in 4x4 texel bounds
            goto ProcessWithCheckingTexelBounds;
        }
        //############################################################
        //ProcessWithoutCheckingTexelBounds:
        //
        // NOTICE: source and target ARE aligned as 4x4 texels
        //
        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            // texel: advance by 4 texels
            uint32_t* texel_x = target;
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 16, texel_x += 4)
            {
                // read DXT3 (BC2) block data
                uint32_t aclut = *(uint32_t*)(source);        // 00-07 : a LUT    (64bits) 4x4x4bits
                uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

                // alpha code and color code [r,g,b] indexes to luts           
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));

                // process 4x4 color code
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width)
                {
                    // TODO: need to test the order of bits in ac

                    for (uint32_t bx = 0; bx < 4; bx++, aclut >>= 4, ccfnlut >>= 2)
                    {

                        // TODO: need to test the order of bits in ac
                        uint32_t ac = (uint32_t)(aclut & 0x0f);
                        uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT3_LUT_COLOR_VALUE_A[ac] |
                                __DXT3_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT3_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT3_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx
                }//by
            }//x_block
        }//y_block
        return;
        //
        //############################################################
        // NOTICE: source and target ARE NOT aligned to 4x4 texels, 
        //         We must check for End Of Image (EOI) in this case.
        //############################################################
        // lazy to write boundary separate processings.
        // Just end of image (EOI) pointer check only.
        // considering that I have encountered few images that are not
        // aligned to 4x4 texels, this should be almost never called.
        // takes ~500us (0.5ms) more time processing 2MB pixel images.
        //############################################################
        //
    ProcessWithCheckingTexelBounds:
        uint32_t* EOI = target + (width * height);
        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            uint32_t* texel_x = target;
            // texel: advance by 4 texels
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 16, texel_x += 4)
            {
                // read DXT3 (BC2) block datas
                uint32_t aclut = *(uint32_t*)(source);        // 00-07 : a LUT    (64bits) 4x4x4bits
                uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

                // alpha code and color code [r,g,b] indexes to lut values           
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));

                // process 4x4 texels
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width)
                {
                    //############################################################
                    // Check Y Bound (break: no more texels available for block)
                    if (texel >= EOI) break;
                    //############################################################
                    for (uint32_t bx = 0; bx < 4; bx++, aclut >>= 4, ccfnlut >>= 2)
                    {
                        //############################################################
                        // Check X Bound (continue: need ac|ccfnlut to complete shift)
                        if (texel + bx >= EOI) continue;
                        //############################################################
                        uint32_t ac = (uint32_t)(aclut & 0x0f);
                        uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT3_LUT_COLOR_VALUE_A[ac] |
                                __DXT3_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT3_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT3_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx
                }//by
            }//x_block
        }//y_block
        DXT3ReleaseLUTs();
    }












    void DXT5ReleaseLUTs()
    {
        if (nullptr != __DXT5_LUT_COLOR_VALUE_A)
        {
            delete[] __DXT5_LUT_COLOR_VALUE_A;
            __DXT5_LUT_COLOR_VALUE_A = nullptr;
        }
        if (nullptr != __DXT5_LUT_COLOR_VALUE_R)
        {
            delete[] __DXT5_LUT_COLOR_VALUE_R;
            __DXT5_LUT_COLOR_VALUE_R = nullptr;
        }
        if (nullptr != __DXT5_LUT_COLOR_VALUE_G)
        {
            delete[] __DXT5_LUT_COLOR_VALUE_G;
            __DXT5_LUT_COLOR_VALUE_B = nullptr;
        }
        if (nullptr != __DXT5_LUT_COLOR_VALUE_B)
        {
            delete[] __DXT5_LUT_COLOR_VALUE_B;
            __DXT5_LUT_COLOR_VALUE_B = nullptr;
        }
    }

    void __DXT5_LUT_COLOR_VALUE_A_Build()
    {
        __DXT5_LUT_COLOR_VALUE_A = new uint32_t[524288]; // 8 * 256 * 256
        // where a[0..7]
        for (int a = 0; a <= 7; a++)
        {
            // where a0[0..255]
            for (int a0 = 0; a0 <= 255; a0++)
            {
                // where a1[0..255]
                for (int a1 = 0; a1 <= 255; a1++)
                {
                    // if (a0 > a1) deliberatelly moved inside switch(a) so it can be switch jump optimized execution
                    int index = (a0 << 3) | (a1 << 11) | (a);
                    switch (a)
                    {
                    case 0:
                        __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)a0 << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    case 1:
                        __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)a1 << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    case 2:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((6 * a0) + (/**/a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((4 * a0) + (/**/a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    case 3:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((5 * a0) + (2 * a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((3 * a0) + (2 * a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    case 4:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((4 * a0) + (3 * a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((2 * a0) + (3 * a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    case 5:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((3 * a0) + (4 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A;
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((/**/a0)+(4 * a1)) / 5)) << __DXT5_LUT_COLOR_SHIFT_A;
                        break;
                    case 6:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((unsigned char)(((2 * a0) + (5 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A;
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = 0;  // __DXT5_LUT_COLOR_SHIFT_A // no point
                        break;
                    case 7:
                        if (a0 > a1)
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)(((unsigned char)(((/**/a0)+(6 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A);
                        else
                            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((255) << __DXT5_LUT_COLOR_SHIFT_A);
                        break;
                    }// switch(a)
                } // a1
            } // a0
        } // a
    }

    // builds static __DXT5_LUT_COLOR_VALUE_RGB[R,G,B] look-up table's
    void __DXT5_LUT_COLOR_VALUE_RGB_Build()
    {
        // DXT5 (BC3) pre calculated values for r & b codes
        unsigned char __DXT5_LUT_4x8[32] = // 0x00 - 0x1f (0-31)
        {
            0,   8,  16,  25,  33,  41,  49,  58,
           66,  74,  82,  90,  99, 107, 115, 123,
          132, 140, 148, 156, 164, 173, 181, 189,
          197, 205, 214, 222, 230, 238, 247, 255
        };

        // DXT5 (BC3) pre calculated values for g codes
        unsigned char __DXT5_LUT_8x8[64] = // 0x00 - 0x3f (0-63)
        {
            0,   4,   8,  12,  16,  20,  24,  28,
           32,  36,  40,  45,  49,  53,  57,  61,
           65,  69,  73,  77,  81,  85,  89,  93,
           97, 101, 105, 109, 113, 117, 121, 125,
          130, 134, 138, 142, 146, 150, 154, 158,
          162, 166, 170, 174, 178, 182, 186, 190,
          194, 198, 202, 206, 210, 214, 219, 223,
          227, 231, 235, 239, 243, 247, 251, 255
        };

        __DXT5_LUT_COLOR_VALUE_R = new uint32_t[4096];  // 4*32*32
        __DXT5_LUT_COLOR_VALUE_G = new uint32_t[16384]; // 4*64*64
        __DXT5_LUT_COLOR_VALUE_B = new uint32_t[4096];  // 4*32*32

        for (int cc0 = 0; cc0 < 32; cc0++)
        {
            for (int cc1 = 0; cc1 < 32; cc1++)
            {
                int index = ((cc0 << 5) | cc1) << 2;
                __DXT5_LUT_COLOR_VALUE_R[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_R);
                __DXT5_LUT_COLOR_VALUE_B[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_B);
                __DXT5_LUT_COLOR_VALUE_R[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_R);
                __DXT5_LUT_COLOR_VALUE_B[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_B);
                // Each RGB image data block is encoded according to the BC1 formats, 
                // with the exception that the two code bits always use the non-transparent encodings. 
                // In other words, they are treated as though color0 > color1, 
                // regardless of the actual values of color0 and color1.   
                // p2 = ((2*c0)+(c1))/3
                __DXT5_LUT_COLOR_VALUE_R[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_4x8[cc0] * 2) + (__DXT5_LUT_4x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_R);
                __DXT5_LUT_COLOR_VALUE_B[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_4x8[cc0] * 2) + (__DXT5_LUT_4x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_B);
                // p3 = ((c0)+(2*c1))/3
                __DXT5_LUT_COLOR_VALUE_R[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_4x8[cc0]) + (__DXT5_LUT_4x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_R);
                __DXT5_LUT_COLOR_VALUE_B[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_4x8[cc0]) + (__DXT5_LUT_4x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_B);
            }
        }
        for (int cc0 = 0; cc0 < 64; cc0++)
        {
            for (int cc1 = 0; cc1 < 64; cc1++)
            {
                int index = ((cc0 << 6) | cc1) << 2;
                __DXT5_LUT_COLOR_VALUE_G[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_8x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_G);
                __DXT5_LUT_COLOR_VALUE_G[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_8x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_G);
                // Each RGB image data block is encoded according to the BC1 formats, 
                // with the exception that the two code bits always use the non-transparent encodings. 
                // In other words, they are treated as though color0 > color1, 
                // regardless of the actual values of color0 and color1.      
                // p2 = ((2*c0)+(c1))/3
                __DXT5_LUT_COLOR_VALUE_G[index | 2] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_8x8[cc0] * 2) + (__DXT5_LUT_8x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_G);
                // p3 = ((c0)+(2*c1))/3
                __DXT5_LUT_COLOR_VALUE_G[index | 3] = (uint32_t)((uint32_t)((unsigned char)(((__DXT5_LUT_8x8[cc0]) + (__DXT5_LUT_8x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_G);
            }
        }
    }

    void S3TC_Decompressor::DXT5Decompress(const unsigned char* p_input)
    {
        if (NULL == __DXT5_LUT_COLOR_VALUE_A)
        {
            __DXT5_LUT_COLOR_VALUE_A_Build();
            __DXT5_LUT_COLOR_VALUE_RGB_Build();
        }

        // direct copy paste from c# code, not even comments changed

        unsigned char* source = (unsigned char*)p_input;
        uint32_t* target = (uint32_t*)image;
        uint32_t target_4scans = (width << 2);
        uint32_t x_block_count = (width + 3) >> 2;
        uint32_t y_block_count = (height + 3) >> 2;

        //############################################################
        if ((x_block_count << 2) != width || (y_block_count << 2) != height)
        {
            // for images that do not fit in 4x4 texel bounds
            goto ProcessWithCheckingTexelBounds;
        }
        //############################################################
        //ProcessWithoutCheckingTexelBounds:
        //
        // NOTICE: source and target ARE aligned as 4x4 texels
        //
        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            // texel: advance by 4 texels
            uint32_t* texel_x = target;
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 16, texel_x += 4)
            {
                // read DXT5 (BC3) block data
                //unsigned char ac0 = *(unsigned char*)(source);            // 00    : a0       (8bit)
                //unsigned char ac1 = *(unsigned char*)(source + 1);        // 01    : a1       (8bit)
                uint32_t acfnlut = *(uint32_t*)(source + 2);  // 02-07 : afn LUT  (48bits) 4x4x3bits
                uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

                // alpha code and color code [r,g,b] indexes to luts           
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));
                //uint32_t ac = ((uint32_t)ac0 << 3) | ((uint32_t)ac1 << 11);
                uint32_t ac = (uint32_t)((*(uint16_t*)source) << 3);

                // process 4x4 color code
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width)
                {
                    for (uint32_t bx = 0; bx < 4; bx++, acfnlut >>= 3, ccfnlut >>= 2)
                    {
                        uint32_t acfn = (uint32_t)(acfnlut & 0x07);
                        uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT5_LUT_COLOR_VALUE_A[ac | acfn] |
                                __DXT5_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT5_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT5_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx
                }//by
            }//x_block
        }//y_block
        return;
        //
        //############################################################
        // NOTICE: source and target ARE NOT aligned to 4x4 texels, 
        //         We must check for End Of Image (EOI) in this case.
        //############################################################
        // lazy to write boundary separate processings.
        // Just end of image (EOI) pointer check only.
        // considering that I have encountered few images that are not
        // aligned to 4x4 texels, this should be almost never called.
        // takes ~500us (0.5ms) more time processing 2MB pixel images.
        //############################################################
        //
    ProcessWithCheckingTexelBounds:
        uint32_t* EOI = target + (width * height);
        // target : advance by 4 scan lines
        for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
        {
            uint32_t* texel_x = target;
            // texel: advance by 4 texels
            for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 16, texel_x += 4)
            {
                // read DXT5 (BC3) block data
                //unsigned char ac0 = *(unsigned char*)(source);            // 00    : a0       (8bit)
                //unsigned char ac1 = *(unsigned char*)(source+ 1);         // 01    : a1       (8bit)
                uint32_t acfnlut = *(uint32_t*)(source + 2);  // 02-07 : afn LUT  (48bits) 4x4x3bits
                uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
                uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
                uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

                // alpha code and color code [r,g,b] indexes to lut values           
                uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
                uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
                uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));
                //uint32_t ac = ((uint32_t)ac0 << 3) | ((uint32_t)ac1 << 11);
                uint32_t ac = (uint32_t)((*(uint16_t*)source) << 3);

                // process 4x4 texels
                uint32_t* texel = texel_x;
                for (uint32_t by = 0; by < 4; by++, texel += width)
                {
                    //############################################################
                    // Check Y Bound (break: no more texels available for block)
                    if (texel >= EOI) break;
                    //############################################################
                    for (uint32_t bx = 0; bx < 4; bx++, acfnlut >>= 3, ccfnlut >>= 2)
                    {
                        //############################################################
                        // Check X Bound (continue: need ac|ccfnlut to complete shift)
                        if (texel + bx >= EOI) continue;
                        //############################################################
                        uint32_t acfn = (uint32_t)(acfnlut & 0x07);
                        uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

                        *(texel + bx) = (uint32_t)
                            (
                                __DXT5_LUT_COLOR_VALUE_A[ac | acfn] |
                                __DXT5_LUT_COLOR_VALUE_R[ccr | ccfn] |
                                __DXT5_LUT_COLOR_VALUE_G[ccg | ccfn] |
                                __DXT5_LUT_COLOR_VALUE_B[ccb | ccfn]
                                );
                    }//bx
                }//by
            }//x_block
        }//y_block

        DXT5ReleaseLUTs();
    }
}