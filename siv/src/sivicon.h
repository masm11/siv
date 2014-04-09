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
 * $Id: sivicon.h 137 2005-03-23 16:35:13Z masm $
 */
#ifndef SIV_ICON_H__INCLUDED
#define SIV_ICON_H__INCLUDED

#include <gtk/gtk.h>

#include "image.h"

#define SIV_TYPE_ICON		  (siv_icon_get_type ())
#define SIV_ICON(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SIV_TYPE_ICON, SivIcon))
#define SIV_ICON_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), SIV_TYPE_ICON, SivIconClass))
#define SIV_IS_ICON(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SIV_TYPE_ICON))
#define SIV_IS_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SIV_TYPE_ICON))
#define SIV_ICON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SIV_TYPE_ICON, SivIconClass))
       

typedef struct _SivIcon       SivIcon;
typedef struct _SivIconClass  SivIconClass;

struct _SivIcon {
    GtkWidget widget;
    
    GdkPixbuf *pixbuf;
};

struct _SivIconClass {
    GtkWidgetClass parent_class;
};

GType                 siv_icon_get_type          (void) G_GNUC_CONST;
GtkWidget*	      siv_icon_new		 (GdkPixbuf *pixbuf);
void		      siv_icon_set_pixbuf	 (SivIcon *icon,
						  GdkPixbuf *pixbuf);

#define ICON_SIZE 64

#endif	/* ifndef SIV_ICON_H__INCLUDED */
