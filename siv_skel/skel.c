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
 * $Id: skel.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <siv/image.h>
#include <siv/format.h>

static struct siv_image_t *skel_read(unsigned char *data, unsigned long size)
{
#if 0
    /* この関数は、引数に与えられたデータを解釈し、
     * 画像を作成する。
     * 作成できたら struct siv_image_t * を malloc() して返し、
     * 作成できなければ NULL を返す。
     */
    
    struct siv_image_t *img;
    
    img = malloc(sizeof *img);
    img->has_alpha = 0;		// data に alpha が存在するかどうか。
    img->width = 0;		// 画像の幅
    img->height = 0;		// 画像の高さ
    
    /* 画像データ
     * 画像左上から右へ。
     * R, G, B, A それぞれ 1バイトで 0〜255。
     * alpha なしの場合は R, G, B, R, G, B, ...
     * alpha ありの場合は R, G, B, A, R, G, B, A, ...
     * A は 0 が透明, 255 が不透明。
     */
    img->data = malloc(img->width * img->height * 3);
    
    /* 今のところ使ってないので、NULL にしておく。
     */
    img->comment = NULL;
    img->short_info = NULL;
    img->long_info = NULL;
    
    if (failed) {
	/* 失敗した場合、image_destroy() で img を解放しておく。
	 * この関数は、
	 *   img->data
	 *   img->comment
	 *   img->short_info
	 *   img->long_info
	 * についても、NULL でなければ解放する。
	 */
	image_destroy(img);
	img = NULL;
    }
    
    return img;
#endif
    
    return NULL;
}

struct siv_image_format_t format = {
    "SKEL",
    skel_read,
};
