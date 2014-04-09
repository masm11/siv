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
 * $Id: mainview.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "pixbuf.h"

static struct {
    GtkWidget *top, *layout, *pix;
    GtkWidget *last_nailview;
    
    gint top_w, top_h;
    gint pix_x, pix_y, pix_w, pix_h;
    gboolean dragging;
    gint orig_x, orig_y;
    gint drag_beg_x, drag_beg_y;
} work;

static void view_file(const gchar *path, GdkPixbuf *pixbuf)
{
    char title[16 + PATH_MAX];
    gint scr_w, scr_h;
    gint pix_w, pix_h;
    gint top_x, top_y, top_w, top_h;
    GdkScreen *scr;
    
    if (path != NULL)
	sprintf(title, "mainview - %s", path);
    else
	strcpy(title, "mainview");
    
    gtk_window_set_title(GTK_WINDOW(work.top), title);
    
    gtk_image_set_from_pixbuf(GTK_IMAGE(work.pix), pixbuf);
    
    scr = gtk_window_get_screen(GTK_WINDOW(work.top));
    scr_w = gdk_screen_get_width(scr);
    scr_h = gdk_screen_get_height(scr);
    
    gtk_window_get_position(GTK_WINDOW(work.top), &top_x, &top_y);
    top_w = pix_w = gdk_pixbuf_get_width(pixbuf);
    top_h = pix_h = gdk_pixbuf_get_height(pixbuf);
    
    if (top_w > scr_w)
	top_w = scr_w;
    if (top_h > scr_h)
	top_h = scr_h;
    
    if (top_x < 0)
	top_x = 0;
    if (top_y < 0)
	top_y = 0;
    if (top_x + top_w > scr_w)
	top_x = scr_w - top_w;
    if (top_y + top_h > scr_h)
	top_y = scr_h - top_h;
    
    work.top_w = top_w;
    work.top_h = top_h;
    work.pix_w = pix_w;
    work.pix_h = pix_h;
    
    if (work.pix_x > 0)
	work.pix_x = 0;
    if (work.pix_x + work.pix_w < work.top_w)
	work.pix_x = work.top_w - work.pix_w;
    if (work.pix_y > 0)
	work.pix_y = 0;
    if (work.pix_y + work.pix_h < work.top_h)
	work.pix_y = work.top_h - work.pix_h;
    
    gtk_layout_set_size(GTK_LAYOUT(work.layout), work.top_w, work.top_h);
    gtk_widget_set_usize(work.layout, work.top_w, work.top_h);
    gtk_layout_move(GTK_LAYOUT(work.layout), work.pix, work.pix_x, work.pix_y);
    
    gtk_window_set_resizable(GTK_WINDOW(work.top), FALSE);
    gtk_window_resize(GTK_WINDOW(work.top), work.top_w, work.top_h);
    gtk_window_move(GTK_WINDOW(work.top), top_x, top_y);
}

static gboolean cb_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    guint mods;
    
    mods = gtk_accelerator_get_default_mod_mask();
    
    if (event->keyval == GDK_q && (event->state & mods) == GDK_CONTROL_MASK) {
	gtk_main_quit();
	return TRUE;
    }
    
    if (event->keyval == GDK_space && (event->state & mods) == 0) {
	if (work.last_nailview != NULL)
	    g_signal_emit_by_name(work.last_nailview, "view_next", 0);
	return TRUE;
    }
    
    if (event->keyval == GDK_BackSpace && (event->state & mods) == 0) {
	if (work.last_nailview != NULL)
	    g_signal_emit_by_name(work.last_nailview, "view_prev", 0);
	return TRUE;
    }
    
    return FALSE;
}

static gboolean cb_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (event->button != 1)
	return FALSE;
    if (event->type != GDK_BUTTON_PRESS)
	return FALSE;
    
    work.dragging = TRUE;
    work.drag_beg_x = event->x_root;
    work.drag_beg_y = event->y_root;
    work.orig_x = work.pix_x;
    work.orig_y = work.pix_y;
    
    return TRUE;
}

static gboolean cb_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
    if (!work.dragging)
	return TRUE;
    
    work.pix_x = work.orig_x + event->x_root - work.drag_beg_x;
    work.pix_y = work.orig_y + event->y_root - work.drag_beg_y;
    
    if (work.pix_x > 0)
	work.pix_x = 0;
    if (work.pix_y > 0)
	work.pix_y = 0;
    if (work.pix_x + work.pix_w < work.top_w)
	work.pix_x = work.top_w - work.pix_w;
    if (work.pix_y + work.pix_h < work.top_h)
	work.pix_y = work.top_h - work.pix_h;
    
    gtk_layout_move(GTK_LAYOUT(work.layout), work.pix, work.pix_x, work.pix_y);
    
    return TRUE;
}

static gboolean cb_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (event->button != 1)
	return FALSE;
    if (event->type != GDK_BUTTON_PRESS)
	return FALSE;
    
    return TRUE;
}

void mainview_create(void)
{
    GdkPixbuf *pixbuf = pixbuf_get_logo();
    
    work.top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_gravity(GTK_WINDOW(work.top), GDK_GRAVITY_STATIC);
    gtk_widget_add_events(work.top, GDK_KEY_PRESS_MASK);
    g_signal_connect(work.top, "key_press_event",
	    G_CALLBACK(cb_key_press_event), NULL);
    
    work.layout = gtk_layout_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(work.top), work.layout);
    gtk_widget_add_events(work.layout, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(work.layout, GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(work.layout, GDK_BUTTON_MOTION_MASK);
    g_signal_connect(work.layout, "button_press_event",
	    G_CALLBACK(cb_button_press_event), NULL);
    g_signal_connect(work.layout, "motion_notify_event",
	    G_CALLBACK(cb_motion_notify_event), NULL);
    g_signal_connect(work.layout, "button_release_event",
	    G_CALLBACK(cb_button_release_event), NULL);
    
    work.pix = gtk_image_new_from_pixbuf(pixbuf);
    gtk_layout_put(GTK_LAYOUT(work.layout), work.pix, 0, 0);
    
    gtk_widget_show_all(work.top);
    
    view_file(NULL, pixbuf);
    
    g_object_unref(pixbuf);
}

void mainview_view(const char *path, GtkWidget *nailview)
{
    GdkPixbuf *pixbuf;
    printf("%s\n", path);
    
    pixbuf = pixbuf_create_image(path);
    if (pixbuf != NULL) {
	view_file(path, pixbuf);
	g_object_unref(pixbuf);
	work.last_nailview = nailview;
    }
}
