#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CS248 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Advanced Task
  // Implement mipmap for trilinear filtering

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8( &mip.texels[i], &c.r );
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 4: Implement nearest neighbour interpolation
  
  if (level != 0)
    // return magenta for invalid level
    return Color(1,0,1,1);

  auto& mip = tex.mipmap[level];
  int su = max(0, min(int(u * mip.width), int(mip.width - 1)));
  int sv = max(0, min(int(v * mip.height), int(mip.height - 1)));
  Color c;
  uint8_to_float(&c.r, &mip.texels[4 * (sv * mip.width + su)]);
  return c;
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 4: Implement bilinear filtering

  float halfu = 0.5f / float(tex.mipmap[level].width);
  float halfv = 0.5f / float(tex.mipmap[level].height);
  Color c00 = sample_nearest(tex, u - halfu, v - halfv, level);
  Color c10 = sample_nearest(tex, u + halfu, v - halfv, level);
  Color c01 = sample_nearest(tex, u - halfu, v + halfv, level);
  Color c11 = sample_nearest(tex, u + halfu, v + halfv, level);

  float uf = u * float(tex.mipmap[level].width) - 0.5f;
  float vf = v * float(tex.mipmap[level].height) - 0.5f;
  uf -= (long)uf;
  vf -= (long)vf;

  Color c0 = c00 * (1.0f - vf) + c01 * vf;
  Color c1 = c10 * (1.0f - vf) + c11 * vf;
  return c0 * (1.0f - uf) + c1 * uf;
}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Advanced Task
  // Implement trilinear filtering

  // return magenta for invalid level
  return Color(1,0,1,1);

}

} // namespace CS248
