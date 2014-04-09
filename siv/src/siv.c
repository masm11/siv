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
 * $Id: siv.c 140 2006-11-22 07:14:26Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk/gdkkeysyms.h>
#include "sivicon.h"
#include "sivnail.h"
#include "sivnailview.h"
#include "pixbuf.h"
#include "mainview.h"

static void view_dir(const gchar *path);
static void cb_open(GtkWidget *widget, const gchar *path, gpointer user_data);
static void cb_view(GtkWidget *widget, const gchar *path, gpointer user_data);
static gboolean cb_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

static void view_dir(const gchar *dir)
{
    GtkWidget *top, *w, *hbox;
    char title[16 + PATH_MAX];
    char path[PATH_MAX];
    
    {
	int fd;
	
	fd = open(".", O_RDONLY);
	if (fd == -1) {
	    perror(".");
	    return;
	}
	
	if (chdir(dir) == -1) {
	    perror(dir);
	    close(fd);
	    return;
	}
	
	if (getcwd(path, sizeof path) == NULL)
	    strcpy(path, ".");
	
	fchdir(fd);
    }
    
    {
	GError *err = NULL;
	gchar *path_disp = g_filename_display_name(path);
	sprintf(title, "siv - %s", path_disp);
	g_free(path_disp);
	if (err != NULL) {
	    printf("%s\n", err->message);
	    g_error_free(err);
	}
    }
    
    top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(top), title);
    gtk_window_set_default_size(GTK_WINDOW(top),
	    (NAIL_WIDTH + SPACING) * 6 + SPACING + 16,
	    (NAIL_HEIGHT + SPACING) * 3 + SPACING);
    gtk_widget_add_events(top, GDK_KEY_PRESS_MASK);
    g_signal_connect(top, "key_press_event",
	    G_CALLBACK(cb_key_press_event), NULL);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(top), hbox);
    
    w = siv_nail_view_new(path, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 0);
    g_signal_connect(w, "open", G_CALLBACK(cb_open), NULL);
    g_signal_connect(w, "view", G_CALLBACK(cb_view), NULL);
    
    w = gtk_vscrollbar_new(siv_nail_view_get_vadjustment(SIV_NAIL_VIEW(w)));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
    
    gtk_widget_show_all(top);
}

static void cb_open(GtkWidget *widget, const gchar *path, gpointer user_data)
{
    struct stat st;
    
    printf("cb_open: path=%s\n", path);
    if (stat(path, &st) == -1) {
	perror(path);
	return;
    }
    
    if (S_ISDIR(st.st_mode)) {
	printf("is dir.\n");
	usleep(500000);
	view_dir(path);
    } else {
	mainview_view(path, widget);
    }
}

static void cb_view(GtkWidget *widget, const gchar *path, gpointer user_data)
{
    struct stat st;
    
    printf("cb_view: path=%s\n", path);
    if (stat(path, &st) == -1) {
	perror(path);
	return;
    }
    
    if (S_ISDIR(st.st_mode)) {
	printf("is dir.\n");
    } else {
	mainview_view(path, widget);
    }
}

static gboolean cb_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    guint mods;
    
    mods = gtk_accelerator_get_default_mod_mask();
    
    if (event->keyval == GDK_w && (event->state & mods) == GDK_CONTROL_MASK) {
	gtk_widget_destroy(widget);
	return TRUE;
    }
    
    if (event->keyval == GDK_q && (event->state & mods) == GDK_CONTROL_MASK) {
	gtk_main_quit();
	return TRUE;
    }
    
    return FALSE;
}

int main(int argc, char **argv)
{
    char cwd[PATH_MAX];
    
    g_thread_init(NULL);
    gdk_threads_init();
    
    gtk_init(&argc, &argv);
    
    image_format_init();
    
    mainview_create();
    
    if (getcwd(cwd, sizeof cwd) == NULL)
	cwd[0] = '\0';
    view_dir(cwd);
    
    gtk_main();
    
    return 0;
}
