/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/cfile.h"
#include "base/clamp.h"
#include "raster/color_scales.h"
#include "raster/image.h"
#include "raster/palette.h"

#include <cstdio>

#define PROCOL_MAGIC_NUMBER     0xB123

namespace raster {
namespace file {

using namespace base;

// Loads a COL file (Animator and Animator Pro format)
Palette* load_col_file(const char* filename)
{
  Palette *pal = NULL;
  int c, r, g, b;
  FILE *f;

  f = fopen(filename, "rb");
  if (!f)
    return NULL;

  // Get file size.
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  div_t d = div(size-8, 3);
  fseek(f, 0, SEEK_SET);

  bool pro = (size == 768)? false: true; // is Animator Pro format?
  if (!(size) || (pro && d.rem)) {       // Invalid format 
    fclose(f);
    return NULL;
  }
  
  // Animator format
  if (!pro) {
    pal = new Palette(FrameNumber(0), 256);

    for (c=0; c<256; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      if (ferror(f))
        break;

      pal->setEntry(c, rgba(scale_6bits_to_8bits(base::clamp(r, 0, 63)),
                            scale_6bits_to_8bits(base::clamp(g, 0, 63)),
                            scale_6bits_to_8bits(base::clamp(b, 0, 63)), 255));
    }
  }
  // Animator Pro format
  else {
    int magic, version;

    fgetl(f);                   // Skip file size
    magic = fgetw(f);           // File format identifier
    version = fgetw(f);         // Version file

    // Unknown format
    if (magic != PROCOL_MAGIC_NUMBER || version != 0) {
      fclose(f);
      return NULL;
    }

    pal = new Palette(FrameNumber(0), MIN(d.quot, 256));

    for (c=0; c<pal->size(); c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      if (ferror(f))
        break;

      pal->setEntry(c, rgba(base::clamp(r, 0, 255),
                            base::clamp(g, 0, 255),
                            base::clamp(b, 0, 255), 255));
    }
  }

  fclose(f);
  return pal;
}

// Saves an Animator Pro COL file
bool save_col_file(const Palette* pal, const char* filename)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
    return false;

  fputl(8+768, f);                 // File size
  fputw(PROCOL_MAGIC_NUMBER, f);   // File format identifier
  fputw(0, f);                     // Version file

  uint32_t c;
  for (int i=0; i<256; i++) {
    c = pal->getEntry(i);

    fputc(rgba_getr(c), f);
    fputc(rgba_getg(c), f);
    fputc(rgba_getb(c), f);
    if (ferror(f))
      break;
  }

  fclose(f);
  return true;
}

} // namespace file
} // namespace raster
