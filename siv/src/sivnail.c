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
 * $Id: sivnail.c 140 2006-11-22 07:14:26Z masm $
 */
#include <stdio.h>
#include <string.h>
#include "sivicon.h"
#include "pixbuf.h"
#include "sivnail.h"

static void siv_nail_class_init    (SivNailClass   *klass);
static void siv_nail_init          (SivNail        *box);
static void siv_nail_destroy(GtkObject *object);
static gboolean siv_nail_expose(GtkWidget *widget, GdkEventExpose *event);
static void siv_nail_add(GtkContainer *container, GtkWidget *widget);
static void siv_nail_remove(GtkContainer *container, GtkWidget *widget);
static void siv_nail_size_request  (GtkWidget      *widget,
				    GtkRequisition *requisition);
static void siv_nail_size_allocate (GtkWidget      *widget,
				    GtkAllocation  *allocation);
static void siv_nail_state_changed (GtkWidget *widget, GtkStateType previous_state);
static void siv_nail_forall(GtkContainer *container,
	gboolean include_internals,
	GtkCallback callback,
	gpointer callback_data);

static void siv_nail_update(SivNail *nail);

static void siv_nail_updater_init(void);
static void siv_nail_updater_add(SivNail *nail);
static void siv_nail_updater_remove(SivNail *nail);
static void siv_nail_updater_prioritize(SivNail *nail);
static void *siv_nail_updater_thread(void *parm);


GType siv_nail_get_type(void)
{
    static GType nail_type = 0;
    
    if (!nail_type) {
	static const GTypeInfo nail_info = {
	    sizeof(SivNailClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) siv_nail_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof(SivNail),
	    0,		/* n_preallocs */
	    (GInstanceInitFunc) siv_nail_init,
	};
	
	nail_type = g_type_register_static(
		GTK_TYPE_CONTAINER, "SivNail",
		&nail_info, 0);
    }
    
    return nail_type;
}

static void siv_nail_class_init(SivNailClass *class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    
    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;
    container_class = (GtkContainerClass *) class;
    
    object_class->destroy = siv_nail_destroy;
    
    widget_class->size_request = siv_nail_size_request;
    widget_class->size_allocate = siv_nail_size_allocate;
    widget_class->state_changed = siv_nail_state_changed;
    widget_class->expose_event = siv_nail_expose;
    
    container_class->add = siv_nail_add;
    container_class->remove = siv_nail_remove;
    container_class->forall = siv_nail_forall;
    
    siv_nail_updater_init();
}

static void siv_nail_init(SivNail *nail)
{
    GTK_WIDGET_SET_FLAGS(nail, GTK_NO_WINDOW);
}

static void siv_nail_destroy(GtkObject *object)
{
    SivNail *nail = SIV_NAIL(object);
    
    siv_nail_updater_remove(nail);
    
    if (nail->icon != NULL) {
	gtk_widget_destroy(nail->icon);
	nail->icon = NULL;
    }
    
    if (nail->label != NULL) {
	gtk_widget_destroy(nail->label);
	nail->label = NULL;
    }
}

GtkWidget *siv_nail_new(const gchar *path)
{
    SivNail *nail;
    GdkPixbuf *pixbuf;
    GtkWidget *icon, *label;
    const char *p;
    
    nail = g_object_new(SIV_TYPE_NAIL, NULL);
    
    nail->path = g_strdup(path);
    
    pixbuf = pixbuf_get_skeleton();
    icon = siv_icon_new(pixbuf);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(nail), icon);
    gtk_widget_show(icon);
    
    p = strrchr(path, '/');
    if (p != NULL)
	p++;
    else
	p = path;
    {
	gchar *p_disp = g_filename_display_name(p);
	label = gtk_label_new(p_disp);
	g_free(p_disp);
    }
    gtk_container_add(GTK_CONTAINER(nail), label);
    gtk_widget_show(label);
    
    siv_nail_updater_add(nail);
    
    return GTK_WIDGET(nail);
}

static gboolean siv_nail_expose(GtkWidget *widget, GdkEventExpose *event)
{
    SivNail *nail;
    GtkContainer *container;
    
    g_return_val_if_fail(SIV_IS_NAIL(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);
    
    container = GTK_CONTAINER(widget);
    nail = SIV_NAIL(widget);
    
    if (GTK_WIDGET_DRAWABLE(widget)) {
	gtk_container_propagate_expose(container, nail->icon, event);
	gtk_container_propagate_expose(container, nail->label, event);
    }
    
    siv_nail_updater_prioritize(nail);
    
    return FALSE;
}

static void siv_nail_add(GtkContainer *container, GtkWidget *widget)
{
    SivNail *nail;
    
    g_return_if_fail(SIV_IS_NAIL(container));
    g_return_if_fail(GTK_IS_WIDGET(widget));
    
    nail = SIV_NAIL(container);
    
    if (SIV_IS_ICON(widget))
	nail->icon = widget;
    
    if (GTK_IS_LABEL(widget))
	nail->label = widget;
    
    gtk_widget_set_parent(widget, GTK_WIDGET(nail));
}

static void siv_nail_remove(GtkContainer *container, GtkWidget *widget)
{
    SivNail *nail;
    
    g_return_if_fail(SIV_IS_NAIL(container));
    
    nail = SIV_NAIL(container);
    
    if (nail->icon == widget)
	nail->icon = NULL;
    
    if (nail->label == widget)
	nail->label = NULL;
    
    gtk_widget_unparent(widget);
}

static void siv_nail_size_request(
	GtkWidget *widget, GtkRequisition *requisition)
{
    SivNail *nail;
    GtkRequisition child_requisition;
    
    nail = SIV_NAIL(widget);
    
    if (nail->icon != NULL)
	gtk_widget_size_request(nail->icon, &child_requisition);
    
    if (nail->label != NULL)
	gtk_widget_size_request(nail->label, &child_requisition);
    
    requisition->width = NAIL_WIDTH;
    requisition->height = NAIL_HEIGHT;
}

static void siv_nail_size_allocate(
	GtkWidget *widget, GtkAllocation *allocation)
{
    SivNail *nail;
    
    nail = SIV_NAIL(widget);
    
    widget->allocation = *allocation;
    
    if (nail->icon != NULL) {
	GtkAllocation child_alloc = *allocation;
	child_alloc.width = ICON_SIZE;
	child_alloc.height = ICON_SIZE;
	gtk_widget_size_allocate(nail->icon, &child_alloc);
    }
    
    if (nail->label != NULL) {
	GtkAllocation child_alloc = *allocation;
	child_alloc.y += ICON_SIZE;
	child_alloc.width = NAIL_WIDTH;
	child_alloc.height = NAIL_HEIGHT - ICON_SIZE;
	gtk_widget_size_allocate(nail->label, &child_alloc);
    }
}

static void siv_nail_state_changed(GtkWidget *widget, GtkStateType previous_state)
{
    SivNail *nail = SIV_NAIL(widget);
    
    if (nail->icon != NULL)
	gtk_widget_set_state(nail->icon, widget->state);
    
    if (nail->label != NULL)
	gtk_widget_set_state(nail->label, widget->state);
}

static void siv_nail_forall(GtkContainer *container,
	gboolean include_internals,
	GtkCallback callback,
	gpointer callback_data)
{
    SivNail *nail;
    
    g_return_if_fail (callback != NULL);
    
    nail = SIV_NAIL(container);
    
    if (include_internals) {
	if (nail->icon != NULL)
	    (*callback)(nail->icon, callback_data);
	if (nail->label != NULL)
	    (*callback)(nail->label, callback_data);
    }
}

const gchar *siv_nail_get_path(SivNail *nail)
{
    return nail->path;
}

void siv_nail_rename(SivNail *nail, const gchar *path)
{
    gchar *p;
    
//    GDK_THREADS_ENTER();
    
    if (nail->path != NULL)
	g_free(nail->path);
    nail->path = g_strdup(path);
    
    p = strrchr(nail->path, '/');
    if (p != NULL)
	p++;
    else
	p = nail->path;
    if (nail->label != NULL)
	gtk_label_set_text(GTK_LABEL(nail->label), p);
    
//    GDK_THREADS_LEAVE();
}

static void siv_nail_update(SivNail *nail)
{
    GdkPixbuf *pixbuf;
    
    pixbuf = pixbuf_create_icon(nail->path);
    
    GDK_THREADS_ENTER();
    
    if (nail->icon != NULL)
	siv_icon_set_pixbuf(SIV_ICON(nail->icon), pixbuf);
    
    GDK_THREADS_LEAVE();
    
    g_object_unref(pixbuf);
}


static struct thread_work_t {
    GMutex *mutex;
    GCond *cond;
    GThread *thread;
    
    GList *list;
    
    gboolean finishing;
} thread_w;

static void siv_nail_updater_init(void)
{
    struct thread_work_t *w = &thread_w;
    
    memset(w, 0, sizeof *w);
    
    w->mutex = g_mutex_new();
    w->cond = g_cond_new();
    w->thread = g_thread_create_full(
	    siv_nail_updater_thread, w, 0,
	    TRUE, TRUE, G_THREAD_PRIORITY_LOW, NULL);
    
    //fixme: free on destroy.
}

static void siv_nail_updater_add(SivNail *nail)
{
    struct thread_work_t *w = &thread_w;
    
    g_mutex_lock(w->mutex);
    
    w->list = g_list_append(w->list, nail);
    g_cond_signal(w->cond);
    
    g_mutex_unlock(w->mutex);
}

static void siv_nail_updater_remove(SivNail *nail)
{
    struct thread_work_t *w = &thread_w;
    
    g_mutex_lock(w->mutex);
    
    w->list = g_list_remove(w->list, nail);
    
    g_mutex_unlock(w->mutex);
}

static void siv_nail_updater_prioritize(SivNail *nail)
{
    struct thread_work_t *w = &thread_w;
    GList *lp;
    
    g_mutex_lock(w->mutex);
    
    lp = g_list_find(w->list, nail);
    if (lp != NULL) {
	w->list = g_list_delete_link(w->list, lp);
	w->list = g_list_prepend(w->list, nail);
    }
    
    g_mutex_unlock(w->mutex);
}

static void *siv_nail_updater_thread(void *parm)
{
    struct thread_work_t *w = parm;
    
    while (TRUE) {
	GtkWidget *nail;
	
	g_mutex_lock(w->mutex);
	
	while (w->list == NULL) {
	    g_cond_wait(w->cond, w->mutex);
	    
	    if (w->finishing) {
		g_mutex_unlock(w->mutex);
		return NULL;
	    }
	}
	
	nail = w->list->data;
	w->list = g_list_delete_link(w->list, w->list);
	
	g_object_ref(nail);
	
	g_mutex_unlock(w->mutex);
	
	siv_nail_update(SIV_NAIL(nail));
	
	g_object_unref(nail);
    }
}
