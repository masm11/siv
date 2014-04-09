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
 * $Id: pixbuf.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "image.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "sivicon.h"		// ICON_SIZE
#include "pixbuf.h"

#include "images/siv_dir.xpm"
#include "images/siv_err.xpm"
#include "images/siv_logo.xpm"
#include "images/siv_skel.xpm"

enum {
    STOCK_LOGO,
    STOCK_LOGO_ICON,
    STOCK_FOLDER,
    STOCK_SKELETON,
    
    STOCK_NR
};

static char **data[STOCK_NR] = {
    siv_logo_xpm,
    siv_err_xpm,
    siv_dir_xpm,
    siv_skel_xpm,
};

static GdkPixbuf *pixbuf[STOCK_NR] = { NULL, };

static GdkPixbuf *pixbuf_get_stock(gint id)
{
    if ((guint) id >= STOCK_NR) {
	fprintf(stderr, "siv: pixbuf_get_stock: bad id %d.", id);
	exit(1);
    }
    
    if (pixbuf[id] == NULL)
	pixbuf[id] = gdk_pixbuf_new_from_xpm_data((const char **) data[id]);
    
    g_object_ref(pixbuf[id]);
    return pixbuf[id];
}

static GdkPixbuf *pixbuf_create_from_image(struct siv_image_t *img)
{
    GdkPixdata pixdata;
    
    if (img->has_alpha) {
	pixdata.magic = GDK_PIXBUF_MAGIC_NUMBER;
	pixdata.length = 0;
	pixdata.pixdata_type =
		GDK_PIXDATA_COLOR_TYPE_RGBA | GDK_PIXDATA_SAMPLE_WIDTH_8 | GDK_PIXDATA_ENCODING_RAW;
	pixdata.rowstride = img->width * 4;
	pixdata.width = img->width;
	pixdata.height = img->height;
	pixdata.pixel_data = img->data;
    } else {
	pixdata.magic = GDK_PIXBUF_MAGIC_NUMBER;
	pixdata.length = 0;
	pixdata.pixdata_type =
		GDK_PIXDATA_COLOR_TYPE_RGB | GDK_PIXDATA_SAMPLE_WIDTH_8 | GDK_PIXDATA_ENCODING_RAW;
	pixdata.rowstride = img->width * 3;
	pixdata.width = img->width;
	pixdata.height = img->height;
	pixdata.pixel_data = img->data;
    }
    
    return gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
}

GdkPixbuf *pixbuf_create_image(const gchar *path)
{
    struct siv_image_t *img;
    GdkPixbuf *pbuf;
    
    img = image_read_file(path);
    if (img == NULL)
	return NULL;
    
    pbuf = pixbuf_create_from_image(img);
    
    image_destroy(img);
    
    return pbuf;
}

GdkPixbuf *pixbuf_create_icon(const gchar *path)
{
    struct stat st;
    GdkPixbuf *pbuf;
    gint orig_w, orig_h;
    
    if (stat(path, &st) == -1)
	return pixbuf_get_stock(STOCK_LOGO_ICON);
    
    if (S_ISDIR(st.st_mode))
	return pixbuf_get_stock(STOCK_FOLDER);
    
    pbuf = pixbuf_create_image(path);
    if (pbuf == NULL)
	return pixbuf_get_stock(STOCK_LOGO_ICON);
    
    orig_w = gdk_pixbuf_get_width(pbuf);
    orig_h = gdk_pixbuf_get_height(pbuf);
    if (orig_w != ICON_SIZE || orig_h != ICON_SIZE) {
	GdkPixbuf *pb;
	gint icon_w, icon_h;
	
	if (orig_w > orig_h) {
	    icon_w = ICON_SIZE;
	    icon_h = ICON_SIZE * orig_h / orig_w;
	} else {
	    icon_w = ICON_SIZE * orig_w / orig_h;
	    icon_h = ICON_SIZE;
	}
	
	pb = gdk_pixbuf_scale_simple(pbuf, icon_w, icon_h, GDK_INTERP_NEAREST);
	g_object_unref(pbuf);
	pbuf = pb;
    }
    
    return pbuf;
}

GdkPixbuf *pixbuf_get_skeleton(void)
{
    return pixbuf_get_stock(STOCK_SKELETON);
}

GdkPixbuf *pixbuf_get_logo(void)
{
    return pixbuf_get_stock(STOCK_LOGO);
}

/*EOF*/
