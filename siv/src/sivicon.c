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
 * $Id: sivicon.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <string.h>
#include "sivicon.h"

static void siv_icon_class_init        (SivIconClass    *klass);
static void siv_icon_init              (SivIcon         *icon);
static void siv_icon_destroy           (GtkObject        *object);
static void siv_icon_finalize          (GObject          *object);
static void siv_icon_size_request      (GtkWidget        *widget,
					GtkRequisition   *requisition);
static void siv_icon_size_allocate     (GtkWidget        *widget,
                                        GtkAllocation    *allocation);
static gint siv_icon_expose            (GtkWidget        *widget,
					GdkEventExpose   *event);
static void siv_icon_realize           (GtkWidget        *widget);
static void siv_icon_unrealize         (GtkWidget        *widget);

static GtkWidgetClass *parent_class = NULL;


GType siv_icon_get_type(void)
{
    static GType icon_type = 0;
    
    if (!icon_type) {
	static const GTypeInfo icon_info = {
	    sizeof(SivIconClass),
	    NULL,           /* base_init */
	    NULL,           /* base_finalize */
	    (GClassInitFunc) siv_icon_class_init,
	    NULL,           /* class_finalize */
	    NULL,           /* class_data */
	    sizeof (SivIcon),
	    32,             /* n_preallocs */
	    (GInstanceInitFunc) siv_icon_init,
	};
	
	icon_type = g_type_register_static(
		GTK_TYPE_WIDGET, "SivIcon", &icon_info, 0);
    }
    
    return icon_type;
}

static void siv_icon_class_init(SivIconClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(class);
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    
    parent_class = g_type_class_peek_parent(class);
    
    gobject_class->finalize = siv_icon_finalize;
    
    object_class->destroy = siv_icon_destroy;
    
    widget_class->size_request = siv_icon_size_request;
    widget_class->size_allocate = siv_icon_size_allocate;
    widget_class->expose_event = siv_icon_expose;
    widget_class->realize = siv_icon_realize;
    widget_class->unrealize = siv_icon_unrealize;
}

static void siv_icon_init(SivIcon *icon)
{
    GTK_WIDGET_SET_FLAGS(icon, GTK_NO_WINDOW);
    
    icon->pixbuf = NULL;
}

GtkWidget* siv_icon_new(GdkPixbuf *pixbuf)
{
    SivIcon *icon;
    
    icon = g_object_new(SIV_TYPE_ICON, NULL);
    
    siv_icon_set_pixbuf(icon, pixbuf);
    
    return GTK_WIDGET(icon);
}

void siv_icon_set_pixbuf(SivIcon *icon, GdkPixbuf *pixbuf)
{
    GdkPixbuf *old;
    
    old = icon->pixbuf;
    icon->pixbuf = pixbuf;
    if (pixbuf != NULL)
	g_object_ref(pixbuf);
    
    gtk_widget_queue_draw(GTK_WIDGET(icon));
    
    if (old != NULL)
	g_object_unref(old);
}

static void siv_icon_destroy(GtkObject *object)
{
    SivIcon *icon = SIV_ICON(object);
    
    (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void siv_icon_finalize(GObject *object)
{
    SivIcon *icon;
    
    g_return_if_fail (SIV_IS_ICON (object));
    
    icon = SIV_ICON(object);
    
    if (icon->pixbuf != NULL) {
	g_object_unref(icon->pixbuf);
	icon->pixbuf = NULL;
    }
    
    (*G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void siv_icon_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    SivIcon *icon;
    
    g_return_if_fail (SIV_IS_ICON (widget));
    g_return_if_fail (requisition != NULL);
    
    icon = SIV_ICON (widget);
    
    requisition->width = ICON_SIZE;
    requisition->height = ICON_SIZE;
}

static void siv_icon_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    SivIcon *icon;
    
    icon = SIV_ICON(widget);
    
    (*GTK_WIDGET_CLASS(parent_class)->size_allocate)(widget, allocation);
}

static gint siv_icon_expose(GtkWidget *widget, GdkEventExpose *event)
{
    SivIcon *icon = SIV_ICON(widget);
    
    if (GTK_WIDGET_MAPPED(widget)) {
	gdk_draw_pixbuf(widget->window,
		widget->style->black_gc,
		icon->pixbuf,
		0, 0,
		widget->allocation.x + (widget->allocation.width - gdk_pixbuf_get_width(icon->pixbuf)) / 2,
		widget->allocation.y + (widget->allocation.height - gdk_pixbuf_get_height(icon->pixbuf)) / 2,
		gdk_pixbuf_get_width(icon->pixbuf),
		gdk_pixbuf_get_height(icon->pixbuf),
		GDK_RGB_DITHER_NONE,
		0, 0);
	
	if (widget->state == GTK_STATE_SELECTED) {
	    gtk_paint_focus(widget->style,
		    widget->window,
		    GTK_STATE_SELECTED,
		    NULL,
		    widget,
		    "siv-icon",
		    widget->allocation.x,
		    widget->allocation.y,
		    widget->allocation.width,
		    widget->allocation.height);
	}
    }
    
    return FALSE;
}

static void siv_icon_realize(GtkWidget *widget)
{
    SivIcon *icon;
    
    icon = SIV_ICON(widget);
    
    (*GTK_WIDGET_CLASS(parent_class)->realize)(widget);
}

static void siv_icon_unrealize(GtkWidget *widget)
{
    SivIcon *icon;
    
    icon = SIV_ICON(widget);
    
    (*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
}
