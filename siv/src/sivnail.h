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
 * $Id: sivnail.h 137 2005-03-23 16:35:13Z masm $
 */
#ifndef SIV_NAIL_H__INCLUDED
#define SIV_NAIL_H__INCLUDED

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "sivicon.h"

#define SIV_TYPE_NAIL            (siv_nail_get_type ())
#define SIV_NAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SIV_TYPE_NAIL, SivNail))
#define SIV_NAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SIV_TYPE_NAIL, SivNailClass))
#define SIV_IS_NAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SIV_TYPE_NAIL))
#define SIV_IS_NAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SIV_TYPE_NAIL))
#define SIV_NAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SIV_TYPE_NAIL, SivNailClass))

#define NAIL_WIDTH	ICON_SIZE
#define NAIL_HEIGHT	(ICON_SIZE + 40)
#define SPACING		8

typedef struct _SivNail        SivNail;
typedef struct _SivNailClass   SivNailClass;

struct _SivNail {
    GtkContainer container;
    
    GtkWidget *icon, *label;
    
    gchar *path;
};

struct _SivNailClass {
    GtkContainerClass parent_class;
};

GType          siv_nail_get_type        (void) G_GNUC_CONST;
GtkWidget*     siv_nail_new             (const gchar *path);
const gchar    *siv_nail_get_path(SivNail *nail);
void siv_nail_rename(SivNail *nail, const gchar *path);

#endif	/* ifndef SIV_NAIL_H__INCLUDED */
