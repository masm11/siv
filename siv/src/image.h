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
 * $Id: image.h 137 2005-03-23 16:35:13Z masm $
 */
#ifndef IMAGE_H__INCLUDED
#define IMAGE_H__INCLUDED

#include <siv/image.h>

struct siv_image_t *image_read_file(const char *fname);
void image_destroy(struct siv_image_t *img);

#endif	/* ifndef IMAGE_H__INCLUDED */
