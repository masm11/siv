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
 * $Id: jpeg.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <siv/image.h>
#include <siv/format.h>

static jmp_buf jmpbuf;

static void error_exit(j_common_ptr cinfo)
{
    char buf[1024];
    (*cinfo->err->format_message)(cinfo, buf);
    fprintf(stderr, "%s\n", buf);
    longjmp(jmpbuf, 1);
}

struct data_src_t {
    struct jpeg_source_mgr pub;
    unsigned char *data;
    unsigned long size;
};

static void init_source(j_decompress_ptr cinfo)
{
    struct data_src_t *p = (struct data_src_t *) cinfo->src;
    cinfo->src->next_input_byte = p->data;
    cinfo->src->bytes_in_buffer = p->size;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
    static unsigned char bytes[2] = { 0xff, JPEG_EOI };
    cinfo->src->next_input_byte = bytes;
    cinfo->src->bytes_in_buffer = 2;
    return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    if (num_bytes <= 0)
	return;
    
    if (cinfo->src->bytes_in_buffer > num_bytes) {
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
    } else {
	cinfo->src->next_input_byte += cinfo->src->bytes_in_buffer;
	cinfo->src->bytes_in_buffer = 0;
    }
}

static void term_source(j_decompress_ptr cinfo)
{
}

static struct siv_image_t *jpeg_read(unsigned char *data, unsigned long size)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct data_src_t src;
    struct siv_image_t *img;
    unsigned char **rows = NULL;
    
    img = malloc(sizeof *img);
    img->has_alpha = 0;		// fixme: これで segv が直ったわけだが、何か他にあるんでは?
    img->width = 0;
    img->height = 0;
    img->data = NULL;
    img->comment = NULL;
    img->short_info = NULL;
    img->long_info = NULL;
    
    memset(&cinfo, 0, sizeof cinfo);
    memset(&jerr, 0, sizeof jerr);
    
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = error_exit;
    
    jpeg_create_decompress(&cinfo);
    
    memset(&src, 0, sizeof src);
    src.pub.init_source = init_source;
    src.pub.fill_input_buffer = fill_input_buffer;
    src.pub.skip_input_data = skip_input_data;
    src.pub.resync_to_restart = jpeg_resync_to_restart;
    src.pub.term_source = term_source;
    src.data = data;
    src.size = size;
    cinfo.src = (struct jpeg_source_mgr *) &src;
    
    if (setjmp(jmpbuf) == 0) {
	int y;
	
	jpeg_read_header(&cinfo, TRUE);
	
	cinfo.out_color_space = JCS_RGB;
	
	jpeg_start_decompress(&cinfo);
	
	img->width = cinfo.output_width;
	img->height = cinfo.output_height;
	img->data = malloc(3 * img->width * img->height);
	if (img->data == NULL) {
	    fprintf(stderr, "out of memory.\n");
	    longjmp(jmpbuf, 1);
	}
	img->short_info = malloc(128);
	img->long_info = malloc(128);
	sprintf(img->short_info, "JPEG %dx%d", img->width, img->height);
	sprintf(img->long_info, "JPEG %dx%d", img->width, img->height);
	
	rows = malloc(sizeof *rows * cinfo.output_height);
	
	rows[0] = img->data;
	for (y = 1; y < cinfo.output_height; y++) {
	    rows[y] = rows[y - 1] + (cinfo.output_components * cinfo.output_width);
	}
	
	while (cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, &rows[cinfo.output_scanline], cinfo.output_height - cinfo.output_scanline);
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	free(rows);
	
	return img;
    } else {
	jpeg_abort_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	image_destroy(img);
	
	if (rows != NULL)
	    free(rows);
	
	return NULL;
    }
}

struct siv_image_format_t format = {
    "JPEG",
    jpeg_read,
};
