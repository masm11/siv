/* Simple Image Viewer
 *  Copyright (C) 2005 Yuuki Harano
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 * $Id: gif.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <gif_lib.h>
#include <siv/image.h>
#include <siv/format.h>

#define COUNTOF(a)	(sizeof(a) / sizeof(a)[0])

static struct interlace_t {
    int offset, skip;
} interlace_table[] = {
    { 0, 8 },
    { 4, 8 },
    { 2, 4 },
    { 1, 2 },
};

struct gif_src_t {
    unsigned char *p;
    unsigned long siz;
};

static int gif_read_data(GifFileType *gtp, GifByteType *buf, int size)
{
    struct gif_src_t *src = gtp->UserData;
    
    if (size > src->siz)
	size = src->siz;
    
    memcpy(buf, src->p, size);
    src->p += size;
    src->siz -= size;
    
    return size;
}

static struct siv_image_t *gif_read(unsigned char *data, unsigned long size)
{
    struct gif_src_t gifsrc;
    GifFileType *gtp = NULL;
    struct siv_image_t *img;
    SavedImage *sip;
    unsigned char *p;
    ColorMapObject *cmo;
    int x, y, bx, by, ex, ey;
    int transparent_index = -1;
    
    gifsrc.p = data;
    gifsrc.siz = size;
    
    img = malloc(sizeof *img);
    img->has_alpha = 0;
    img->width = 0;
    img->height = 0;
    img->data = NULL;
    img->comment = NULL;
    img->short_info = NULL;
    img->long_info = NULL;
    
    gtp = DGifOpen(&gifsrc, gif_read_data);
    if (gtp == NULL)
	goto err;
    
    if (DGifSlurp(gtp) != GIF_OK)
	goto err;
    
    if (gtp->ImageCount < 1) {
	fprintf(stderr, "gif: no image.\n");
	goto err;
    }
    sip = gtp->SavedImages;
    cmo = gtp->SColorMap;
    if (sip->ImageDesc.ColorMap != NULL)
	cmo = sip->ImageDesc.ColorMap;
    if (cmo == NULL) {
	fprintf(stderr, "gif: no colormap.\n");
	goto err;
    }
    
    img->has_alpha = 1;
    img->width = gtp->SWidth;
    img->height = gtp->SHeight;
    img->data = malloc(gtp->SWidth * gtp->SHeight * 4);
    
    p = img->data;
    for (y = 0; y < img->height; y++) {
	for (x = 0; x < img->width; x++) {
	    *p++ = cmo->Colors[gtp->SBackGroundColor].Red;
	    *p++ = cmo->Colors[gtp->SBackGroundColor].Green;
	    *p++ = cmo->Colors[gtp->SBackGroundColor].Blue;
	    *p++ = 255;
	}
    }
    
    {
	int ei, i;
	for (ei = 0; ei < sip->ExtensionBlockCount; ei++) {
	    if (sip->ExtensionBlocks[ei].Function == GRAPHICS_EXT_FUNC_CODE) {
		if (sip->ExtensionBlocks[ei].ByteCount >= 4) {
		    if (sip->ExtensionBlocks[ei].Bytes[0] & 0x01)
			transparent_index = (unsigned char) sip->ExtensionBlocks[ei].Bytes[3];
		}
	    }
	}
    }
    
    bx = sip->ImageDesc.Left;
    ex = bx + sip->ImageDesc.Width;
    by = sip->ImageDesc.Top;
    ey = by + sip->ImageDesc.Height;;
    
    if (sip->ImageDesc.Interlace) {
	int pass;
	p = sip->RasterBits;
	for (pass = 0; pass < COUNTOF(interlace_table); pass++) {
	    int offset = interlace_table[pass].offset;
	    int skip = interlace_table[pass].skip;
	    for (y = by + offset; y < ey; y += skip) {
		unsigned char *dp = img->data + (y * img->width + bx) * 4;
		for (x = bx; x < ex; x++) {
		    int c = *p++;
		    *dp++ = cmo->Colors[c].Red;
		    *dp++ = cmo->Colors[c].Green;
		    *dp++ = cmo->Colors[c].Blue;
		    *dp++ = c == transparent_index ? 0x00 : 0xff;
		}
	    }
	}
    } else {
	p = sip->RasterBits;
	for (y = by; y < ey; y++) {
	    unsigned char *dp = img->data + (y * img->width + bx) * 4;
	    for (x = bx; x < ex; x++) {
		int c = *p++;
		*dp++ = cmo->Colors[c].Red;
		*dp++ = cmo->Colors[c].Green;
		*dp++ = cmo->Colors[c].Blue;
		*dp++ = c == transparent_index ? 0x00 : 0xff;
	    }
	}
    }
    
    if (gtp != NULL)
	DGifCloseFile(gtp);
    
    return img;
    
 err:
    if (gtp != NULL)
	DGifCloseFile(gtp);
    if (img != NULL)
	image_destroy(img);
    
    return NULL;
}

struct siv_image_format_t format = {
    "GIF",
    gif_read,
};
