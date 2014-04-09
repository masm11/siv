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
 * $Id: sivnailview.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sivnail.h"
#include "sivnailview.h"
#include <gtk/gtkbindings.h>
#include <gdk/gdkkeysyms.h>

#define COUNTOF(a) (sizeof(a) / sizeof(a)[0])

enum {
    SELECT,
    VIEW,
    OPEN,
    
    VIEW_PREV,
    VIEW_NEXT,
    SEL_PREV,
    SEL_NEXT,
    SEL_UP,
    SEL_DOWN,
    OPEN_CUR,
    
    LAST_SIGNAL
};

static inline gint nrows_to_height(gint nrows)
{
    if (nrows < 1)
	return 0;
    return nrows * (NAIL_HEIGHT + SPACING) + SPACING;
}

static inline gint width_to_ncols(gint width)
{
    return (width - SPACING) / (NAIL_WIDTH + SPACING);
}

typedef struct _SivNailViewChild   SivNailViewChild;

struct _SivNailViewChild {
    GtkWidget *widget;
    gint x;
    gint y;
};

enum {
    PROP_0,
    PROP_VADJUSTMENT,
};

static void siv_nail_view_class_init         (SivNailViewClass *class);
static void siv_nail_view_get_property       (GObject        *object,
                                              guint           prop_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);
static void siv_nail_view_set_property       (GObject        *object,
                                              guint           prop_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static GObject *siv_nail_view_constructor    (GType                  type,
					      guint                  n_properties,
					      GObjectConstructParam *properties);
static void siv_nail_view_init               (SivNailView      *nail_view);
static void siv_nail_view_finalize           (GObject        *object);
static void siv_nail_view_realize            (GtkWidget      *widget);
static void siv_nail_view_unrealize          (GtkWidget      *widget);
static void siv_nail_view_map                (GtkWidget      *widget);
static void siv_nail_view_size_request       (GtkWidget      *widget,
                                              GtkRequisition *requisition);
static void siv_nail_view_size_allocate      (GtkWidget      *widget,
                                              GtkAllocation  *allocation);
static gint siv_nail_view_expose             (GtkWidget      *widget,
                                              GdkEventExpose *event);
static void siv_nail_view_remove             (GtkContainer   *container,
                                              GtkWidget      *widget);
static void siv_nail_view_forall             (GtkContainer   *container,
                                              gboolean        include_internals,
                                              GtkCallback     callback,
                                              gpointer        callback_data);
static void siv_nail_view_add(GtkContainer *container, GtkWidget *widget);
static void siv_nail_view_set_adjustments    (SivNailView      *nail_view,
                                              GtkAdjustment  *vadj);
static void siv_nail_view_allocate_children(SivNailView *nail_view);
static void siv_nail_view_allocate_child     (SivNailView      *nail_view,
                                              SivNailViewChild *child);
static void siv_nail_view_adjustment_changed (GtkAdjustment  *adjustment,
                                              SivNailView      *nail_view);
static void siv_nail_view_set_adjustment_upper (GtkAdjustment *adj,
					        gdouble        upper,
					        gboolean       always_emit_changed);

static void siv_nail_view_view_prev(GtkWidget *widget);
static void siv_nail_view_view_next(GtkWidget *widget);
static void siv_nail_view_sel_prev(GtkWidget *widget);
static void siv_nail_view_sel_next(GtkWidget *widget);
static void siv_nail_view_sel_up(GtkWidget *widget);
static void siv_nail_view_sel_down(GtkWidget *widget);
static void siv_nail_view_open_cur(GtkWidget *widget);

static gboolean siv_nail_view_button_press_event(
	GtkWidget *widget, GdkEventButton *event);
static gboolean siv_nail_view_button_release_event(
	GtkWidget *widget, GdkEventButton *event);
static gboolean siv_nail_view_motion_notify_event(
	GtkWidget *widget, GdkEventMotion *event);
static gboolean siv_nail_view_scroll_event(GtkWidget *widget, GdkEventScroll *event);

static void siv_nail_view_drag_data_get(GtkWidget *widget,
	GdkDragContext *dc, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data);
static void siv_nail_view_drag_data_delete(GtkWidget *widget,
	GdkDragContext *dc, gpointer data);
static gboolean siv_nail_view_drag_drop(GtkWidget *widget,
	GdkDragContext *dc, gint x, gint y,
	guint t, gpointer data);
static void siv_nail_view_drag_data_received(GtkWidget *widget,
	GdkDragContext *dc, gint x, gint y,
	GtkSelectionData *selection_data,
	guint info, guint t, gpointer data);
static gboolean copy_file(const gchar *src, const gchar *dstdir);

static GtkWidgetClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

static GtkTargetEntry drop_types[] = { { "application/x-siv", 0, 0 } };

GtkWidget *siv_nail_view_new(const gchar *dirpath, GtkAdjustment *vadjustment)
{
    SivNailView *nail_view;
    GtkContainer *container;
    GDir *dir;
    
    nail_view = g_object_new(SIV_TYPE_NAIL_VIEW,
	    "vadjustment", vadjustment,
	    NULL);
    
    container = GTK_CONTAINER(nail_view);
    
    nail_view->dirpath = g_strdup(dirpath);
    
    dir = g_dir_open(dirpath, 0, NULL);
    if (dir != NULL) {
	GList *list = NULL, *lp;
	const gchar *name;
	
	while ((name = g_dir_read_name(dir)) != NULL) {
	    list = g_list_append(list, g_strdup(name));
	}
	
	g_dir_close(dir);
	
	list = g_list_sort(list, (GCompareFunc) strcmp);
	
	list = g_list_prepend(list, g_strdup(".."));
	
	for (lp = list; lp != NULL; lp = lp->next) {
	    gchar path[PATH_MAX];
	    GtkWidget *icon;
	    
	    sprintf(path, "%s/%s", dirpath, (gchar *) lp->data);
	    icon = siv_nail_new(path);
	    gtk_container_add(container, icon);
	    gtk_widget_show(icon);
	}
    }
    
    return GTK_WIDGET(nail_view);
}

GtkAdjustment *siv_nail_view_get_vadjustment(SivNailView *nail_view)
{
    g_return_val_if_fail(SIV_IS_NAIL_VIEW(nail_view), NULL);
    
    return nail_view->vadjustment;
}

static GtkAdjustment *new_default_adjustment(void)
{
    return GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

static void siv_nail_view_set_adjustments(
	SivNailView *nail_view, GtkAdjustment *vadj)
{
    gboolean need_adjust = FALSE;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(nail_view));
    
    if (vadj)
	g_return_if_fail(GTK_IS_ADJUSTMENT(vadj));
    else if (nail_view->vadjustment)
	vadj = new_default_adjustment();
    
    if (nail_view->vadjustment && nail_view->vadjustment != vadj) {
	g_signal_handlers_disconnect_by_func(nail_view->vadjustment,
		siv_nail_view_adjustment_changed,
		nail_view);
	g_object_unref(nail_view->vadjustment);
    }
    
    if (nail_view->vadjustment != vadj) {
	nail_view->vadjustment = vadj;
	g_object_ref(nail_view->vadjustment);
	gtk_object_sink(GTK_OBJECT(nail_view->vadjustment));
	siv_nail_view_set_adjustment_upper(nail_view->vadjustment,
		nrows_to_height(nail_view->nrows), FALSE);
	
	g_signal_connect(nail_view->vadjustment, "value_changed",
		G_CALLBACK(siv_nail_view_adjustment_changed),
		nail_view);
	need_adjust = TRUE;
    }
    
    /* vadj can be NULL while constructing; don't emit a signal then */
    if (need_adjust && vadj)
	siv_nail_view_adjustment_changed(NULL, nail_view);
}

static void siv_nail_view_finalize(GObject *object)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(object);
    
    g_object_unref(nail_view->vadjustment);
    
    g_free(nail_view->dirpath);
    
    (*G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void siv_nail_view_add(GtkContainer *container, GtkWidget *widget)
{
    SivNailView *nail_view;
    SivNailViewChild *child;
    guint num;
    gint x, y;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(container));
    g_return_if_fail(GTK_IS_WIDGET(widget));
    
    nail_view = SIV_NAIL_VIEW(container);
    
    num = g_list_length(nail_view->children);
    x = num % nail_view->ncols * (NAIL_WIDTH + SPACING) + SPACING;
    y = num / nail_view->ncols * (NAIL_HEIGHT + SPACING) + SPACING;
    
    child = g_new(SivNailViewChild, 1);
    
    child->widget = widget;
    child->x = x;
    child->y = y;
    
    nail_view->children = g_list_append(nail_view->children, child);
    
    if (GTK_WIDGET_REALIZED(GTK_WIDGET(nail_view))) {
//	printf("%p->%p\n", child->widget->window, nail_view->subwin);
	gtk_widget_set_parent_window(child->widget, nail_view->subwin);
    }
    
    gtk_widget_set_parent(widget, GTK_WIDGET(nail_view));
    
    siv_nail_view_allocate_child(nail_view, child);
}

void siv_nail_view_set_vadjustment(
	SivNailView *nail_view, GtkAdjustment *adjustment)
{
    g_return_if_fail(SIV_IS_NAIL_VIEW(nail_view));
    
    siv_nail_view_set_adjustments(nail_view, adjustment);
    g_object_notify(G_OBJECT(nail_view), "vadjustment");
}

static SivNailViewChild *get_child(SivNailView *nail_view, GtkWidget *widget)
{
    GList *children;
    
    for (children = nail_view->children; children != NULL; children = children->next) {
	SivNailViewChild *child = children->data;
	if (child->widget == widget)
	    return child;
    }
    
    return NULL;
}

static void siv_nail_view_set_adjustment_upper(
	GtkAdjustment *adj,
	gdouble upper,
	gboolean always_emit_changed)
{
    gboolean changed = FALSE;
    gboolean value_changed = FALSE;
    
    gdouble min = MAX(0., upper - adj->page_size);
    
    if (upper != adj->upper) {
	adj->upper = upper;
	changed = TRUE;
    }
    
    if (adj->value > min) {
	adj->value = min;
	value_changed = TRUE;
    }
    
    if (changed || always_emit_changed)
	gtk_adjustment_changed(adj);
    if (value_changed)
	gtk_adjustment_value_changed(adj);
}

void siv_nail_view_freeze(SivNailView *nail_view)
{
    g_return_if_fail(SIV_IS_NAIL_VIEW(nail_view));
    
    nail_view->freeze_count++;
}

void siv_nail_view_thaw(SivNailView *nail_view)
{
    g_return_if_fail(SIV_IS_NAIL_VIEW(nail_view));
    
    if (nail_view->freeze_count) {
	if (--nail_view->freeze_count == 0) {
	    gtk_widget_queue_draw(GTK_WIDGET(nail_view));
	    gdk_window_process_updates(GTK_WIDGET(nail_view)->window, TRUE);
	}
    }
}

GType siv_nail_view_get_type(void)
{
    static GType nail_view_type = 0;
    
    if (!nail_view_type) {
	static const GTypeInfo nail_view_info = {
	    sizeof(SivNailViewClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) siv_nail_view_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof(SivNailView),
	    0,		/* n_preallocs */
	    (GInstanceInitFunc) siv_nail_view_init,
	};
	
	nail_view_type = g_type_register_static(
		GTK_TYPE_CONTAINER, "SivNailView",
		&nail_view_info, 0);
    }
    
    return nail_view_type;
}

static void siv_nail_view_class_init(SivNailViewClass *class)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GtkBindingSet *binding_set;
    
    gobject_class = (GObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;
    container_class = (GtkContainerClass*) class;
    
    parent_class = g_type_class_peek_parent(class);
    
    gobject_class->finalize = siv_nail_view_finalize;
    gobject_class->constructor = siv_nail_view_constructor;
    gobject_class->set_property = siv_nail_view_set_property;
    gobject_class->get_property = siv_nail_view_get_property;
    
    widget_class->button_press_event = siv_nail_view_button_press_event;
    widget_class->button_release_event = siv_nail_view_button_release_event;
    widget_class->motion_notify_event = siv_nail_view_motion_notify_event;
    widget_class->scroll_event = siv_nail_view_scroll_event;
    
    container_class->add = siv_nail_view_add;
    
    g_object_class_install_property(gobject_class,
	    PROP_VADJUSTMENT,
	    g_param_spec_object("vadjustment",
		    "Vertical adjustment",
		    "The GtkAdjustment for the vertical position",
		    GTK_TYPE_ADJUSTMENT,
		    G_PARAM_READWRITE));
    
    widget_class->realize = siv_nail_view_realize;
    widget_class->unrealize = siv_nail_view_unrealize;
    widget_class->map = siv_nail_view_map;
    widget_class->size_request = siv_nail_view_size_request;
    widget_class->size_allocate = siv_nail_view_size_allocate;
    widget_class->expose_event = siv_nail_view_expose;
    
    container_class->remove = siv_nail_view_remove;
    container_class->forall = siv_nail_view_forall;
    
    class->set_scroll_adjustments = siv_nail_view_set_adjustments;
    class->view_prev = siv_nail_view_view_prev;
    class->view_next = siv_nail_view_view_next;
    class->sel_prev = siv_nail_view_sel_prev;
    class->sel_next = siv_nail_view_sel_next;
    class->sel_up = siv_nail_view_sel_up;
    class->sel_down = siv_nail_view_sel_down;
    class->open_cur = siv_nail_view_open_cur;
    
    signals[SELECT] = g_signal_new(
	    "select",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST,
	    0,
	    NULL, NULL,
	    g_cclosure_marshal_VOID__STRING,
	    G_TYPE_NONE, 1,
	    G_TYPE_STRING);
    
    signals[VIEW] = g_signal_new(
	    "view",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST,
	    0,
	    NULL, NULL,
	    g_cclosure_marshal_VOID__STRING,
	    G_TYPE_NONE, 1,
	    G_TYPE_STRING);
    
    signals[OPEN] = g_signal_new(
	    "open",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST,
	    0,
	    NULL, NULL,
	    g_cclosure_marshal_VOID__STRING,
	    G_TYPE_NONE, 1,
	    G_TYPE_STRING);
    
    signals[VIEW_PREV] = g_signal_new(
	    "view_prev",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, view_prev),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[VIEW_NEXT] = g_signal_new(
	    "view_next",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, view_next),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[SEL_PREV] = g_signal_new(
	    "sel_prev",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, sel_prev),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[SEL_NEXT] = g_signal_new(
	    "sel_next",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, sel_next),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[SEL_UP] = g_signal_new(
	    "sel_up",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, sel_up),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[SEL_DOWN] = g_signal_new(
	    "sel_down",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, sel_down),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    signals[OPEN_CUR] = g_signal_new(
	    "open_cur",
	    G_OBJECT_CLASS_TYPE(class),
	    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	    G_STRUCT_OFFSET(SivNailViewClass, open_cur),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__VOID,
	    G_TYPE_NONE, 0);
    
    binding_set = gtk_binding_set_by_class(class);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_BackSpace, 0,
	    "view_prev", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_space, 0,
	    "view_next", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_Left, 0,
	    "sel_prev", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_Right, 0,
	    "sel_next", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_Up, 0,
	    "sel_up", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_Down, 0,
	    "sel_down", 0);
    gtk_binding_entry_add_signal(binding_set,
	    GDK_Return, 0,
	    "open_cur", 0);
    
#if 0
    widget_class->set_scroll_adjustments_signal =
	    g_signal_new("set_scroll_adjustments",
		    G_OBJECT_CLASS_TYPE(gobject_class),
		    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(SivNailViewClass, set_scroll_adjustments),
		    NULL, NULL,
		    _gtk_marshal_VOID__OBJECT_OBJECT,
		    G_TYPE_NONE, 2,
		    GTK_TYPE_ADJUSTMENT,
		    GTK_TYPE_ADJUSTMENT);
#endif
}

static void siv_nail_view_get_property(GObject *object,
	guint        prop_id,
	GValue      *value,
	GParamSpec  *pspec)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(object);
    
    switch (prop_id) {
    case PROP_VADJUSTMENT:
	g_value_set_object(value, nail_view->vadjustment);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void siv_nail_view_set_property(GObject *object,
	guint prop_id,
	const GValue *value,
	GParamSpec *pspec)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(object);
    
    switch (prop_id) {
    case PROP_VADJUSTMENT:
	siv_nail_view_set_vadjustment(nail_view, 
		(GtkAdjustment*) g_value_get_object(value));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void siv_nail_view_init(SivNailView *nail_view)
{
    nail_view->children = NULL;
    
    nail_view->ncols = 1;
    nail_view->nrows = 1;
    
    nail_view->vadjustment = NULL;
    
    nail_view->freeze_count = 0;
    
    nail_view->dnd_target_list = gtk_target_list_new(drop_types, COUNTOF(drop_types));
    
    gtk_drag_dest_set(GTK_WIDGET(nail_view),
	    GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
	    drop_types, COUNTOF(drop_types),
	    GDK_ACTION_MOVE);
    
    g_signal_connect(nail_view, "drag_data_get",
	    G_CALLBACK(siv_nail_view_drag_data_get), nail_view);
    g_signal_connect(nail_view, "drag_data_delete",
	    G_CALLBACK(siv_nail_view_drag_data_delete), nail_view);
    g_signal_connect(nail_view, "drag_drop",
	    G_CALLBACK(siv_nail_view_drag_drop), nail_view);
    g_signal_connect(nail_view, "drag_data_received",
	    G_CALLBACK(siv_nail_view_drag_data_received), nail_view);
    
    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(nail_view), GTK_CAN_FOCUS);
    
    gtk_widget_add_events(GTK_WIDGET(nail_view), GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(GTK_WIDGET(nail_view), GDK_BUTTON_PRESS_MASK);
}

static GObject *siv_nail_view_constructor(
	GType type,
	guint n_properties,
	GObjectConstructParam *properties)
{
    SivNailView *nail_view;
    GObject *object;
    GtkAdjustment *vadj;
    
    object = (*G_OBJECT_CLASS(parent_class)->constructor)(
	    type, n_properties, properties);
    
    nail_view = SIV_NAIL_VIEW(object);
    
    vadj = nail_view->vadjustment ? nail_view->vadjustment : new_default_adjustment();
    
    if (!nail_view->vadjustment)
	siv_nail_view_set_adjustments(nail_view, vadj);
    
    return object;
}

static void siv_nail_view_realize(GtkWidget *widget)
{
    SivNailView *nail_view;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkWindow *win;
    GList *lp;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(widget));
    
    nail_view = SIV_NAIL_VIEW(widget);
    GTK_WIDGET_SET_FLAGS(nail_view, GTK_REALIZED);
    
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);
    
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    
    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
	    &attributes, attributes_mask);
    gdk_window_set_user_data(widget->window, widget);
    
    widget->style = gtk_style_attach(widget->style, widget->window);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
    
    
    attributes.x = 0;
    attributes.y = -nail_view->vadjustment->value;
    attributes.width = MAX(widget->allocation.width,
	    (NAIL_WIDTH + SPACING) * nail_view->ncols + SPACING);
    attributes.height = MAX(widget->allocation.height,
	    (NAIL_HEIGHT + SPACING) * nail_view->nrows + SPACING);
    
    attributes.event_mask =
	    GDK_EXPOSURE_MASK
	    | GDK_BUTTON_PRESS_MASK
	    | GDK_BUTTON_RELEASE_MASK
	    | GDK_BUTTON_MOTION_MASK
	    | GDK_SCROLL_MASK
	    | gtk_widget_get_events(widget);
    
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    
    win = gdk_window_new(widget->window, &attributes, attributes_mask);
    gdk_window_set_user_data(win, widget);
    
    gtk_style_set_background(widget->style, win, GTK_STATE_NORMAL);
    
    nail_view->subwin = win;
    
    for (lp = nail_view->children; lp != NULL; lp = lp->next) {
	SivNailViewChild *child = lp->data;
	gtk_widget_set_parent_window(child->widget, nail_view->subwin);
    }
    
    siv_nail_view_allocate_children(nail_view);
}

static void siv_nail_view_map(GtkWidget *widget)
{
    GList *tmp_list;
    SivNailView *nail_view;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(widget));
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);
    
    tmp_list = nail_view->children;
    while (tmp_list) {
	SivNailViewChild *child = tmp_list->data;
	tmp_list = tmp_list->next;
	
	if (GTK_WIDGET_VISIBLE(child->widget)) {
	    if (!GTK_WIDGET_MAPPED(child->widget))
		gtk_widget_map(child->widget);
	}
    }
    
    gdk_window_show(nail_view->subwin);
    gdk_window_show(widget->window);
}

static void siv_nail_view_unrealize(GtkWidget *widget)
{
    SivNailView *nail_view;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(widget));
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    gdk_window_set_user_data(nail_view->subwin, NULL);
    gdk_window_destroy(nail_view->subwin);
    nail_view->subwin = NULL;
    
    (*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
}

static void siv_nail_view_size_request(
	GtkWidget *widget, GtkRequisition *requisition)
{
    SivNailView *nail_view;
    GList *lp;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(widget));
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    for (lp = nail_view->children; lp != NULL; lp = lp->next) {
	SivNailViewChild *child = lp->data;
	GtkRequisition child_requisition;
	gtk_widget_size_request(child->widget, &child_requisition);
    }
    
    requisition->width = 1;
    requisition->height = 1;
}

static void siv_nail_view_size_allocate(
	GtkWidget *widget, GtkAllocation *allocation)
{
    SivNailView *nail_view;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(widget));
    
    widget->allocation = *allocation;
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    nail_view->ncols = width_to_ncols(allocation->width);
    if (nail_view->ncols < 1)
	nail_view->ncols = 1;
    nail_view->nrows = (g_list_length(nail_view->children) + nail_view->ncols - 1) / nail_view->ncols;
    
    siv_nail_view_allocate_children(nail_view);
    
    if (GTK_WIDGET_REALIZED(widget)) {
	gdk_window_move_resize(widget->window,
		allocation->x, allocation->y,
		allocation->width, allocation->height);

	gdk_window_resize(nail_view->subwin,
		MAX(widget->allocation.width,
			(NAIL_WIDTH + SPACING) * nail_view->ncols + SPACING),
		MAX(widget->allocation.height,
			(NAIL_HEIGHT + SPACING) * nail_view->nrows + SPACING));
    }
    
    nail_view->vadjustment->page_size = allocation->height;
    nail_view->vadjustment->page_increment = allocation->height * 0.5;
    nail_view->vadjustment->lower = 0;
    nail_view->vadjustment->upper = nrows_to_height(nail_view->nrows);
    siv_nail_view_set_adjustment_upper(nail_view->vadjustment, nrows_to_height(nail_view->nrows), TRUE);
}

static gint siv_nail_view_expose(GtkWidget *widget, GdkEventExpose *event)
{
    SivNailView *nail_view;
    
    g_return_val_if_fail(SIV_IS_NAIL_VIEW(widget), FALSE);
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    if (nail_view->subwin != event->window)
	return FALSE;
    
    (*GTK_WIDGET_CLASS(parent_class)->expose_event)(widget, event);
    
    return FALSE;
}

static void siv_nail_view_remove(
	GtkContainer *container, GtkWidget *widget)
{
    SivNailView *nail_view;
    SivNailViewChild *child = NULL;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(container));
    
    nail_view = SIV_NAIL_VIEW(container);
    
    child = get_child(nail_view, widget);
    
    nail_view->selection = g_list_remove_all(nail_view->selection, child);
    
    nail_view->children = g_list_remove_all(nail_view->children, child);
    
    gtk_widget_unparent(child->widget);
    
    siv_nail_view_allocate_children(nail_view);
}

static void siv_nail_view_forall(
	GtkContainer *container,
	gboolean include_internals,
	GtkCallback callback,
	gpointer callback_data)
{
    SivNailView *nail_view;
    SivNailViewChild *child;
    GList *tmp_list;
    
    g_return_if_fail(SIV_IS_NAIL_VIEW(container));
    g_return_if_fail(callback != NULL);
    
    nail_view = SIV_NAIL_VIEW(container);
    
    tmp_list = nail_view->children;
    while (tmp_list) {
	child = tmp_list->data;
	tmp_list = tmp_list->next;
	
#if 0
	printf("siv_nail_view_forall: cb=%p, path=%s\n",
		callback, siv_nail_get_path(SIV_NAIL(child->widget)));
#endif
	(*callback)(child->widget, callback_data);
    }
}

static void siv_nail_view_allocate_children(SivNailView *nail_view)
{
    GList *list;
    int x, y;
    
    list = nail_view->children;
    x = y = 0;
    while (list != NULL) {
	SivNailViewChild *child = list->data;
	list = list->next;
	
	child->x = SPACING + (NAIL_WIDTH + SPACING) * x;
	child->y = SPACING + (NAIL_HEIGHT + SPACING) * y;
	
	if (++x >= nail_view->ncols) {
	    x = 0;
	    y++;
	}
	
	siv_nail_view_allocate_child(nail_view, child);
    }
}

static void siv_nail_view_allocate_child(SivNailView *nail_view, SivNailViewChild *child)
{
    GtkAllocation allocation;
    
    if (!GTK_WIDGET_REALIZED(GTK_WIDGET(nail_view)))
	return;
    
    allocation.x = child->x;
    allocation.y = child->y;
    allocation.width = NAIL_WIDTH;
    allocation.height = NAIL_HEIGHT;
    
    gtk_widget_size_allocate(child->widget, &allocation);
}

static void siv_nail_view_adjustment_changed(
	GtkAdjustment *adjustment, SivNailView *nail_view)
{
    GtkWidget *widget;
    
    widget = GTK_WIDGET(nail_view);
    
    if (nail_view->freeze_count)
	return;
    
    if (GTK_WIDGET_REALIZED(nail_view)) {
	gdk_window_move(nail_view->subwin, 0, -nail_view->vadjustment->value);
	gdk_window_process_updates(nail_view->subwin, TRUE);
    }
}

static void unselect_all(SivNailView *nail_view)
{
    while (nail_view->selection) {
	SivNailViewChild *child = nail_view->selection->data;
	nail_view->selection = g_list_delete_link(nail_view->selection, nail_view->selection);
	
	gtk_widget_set_state(child->widget, GTK_STATE_NORMAL);
    }
}

static void select_add(SivNailView *nail_view, SivNailViewChild *child)
{
    nail_view->selection = g_list_append(nail_view->selection, child);
    
    gtk_widget_set_state(child->widget, GTK_STATE_SELECTED);
}

static void select_single(SivNailView *nail_view, SivNailViewChild *child)
{
    unselect_all(nail_view);
    select_add(nail_view, child);
}

static void select_multi(SivNailView *nail_view,
	gint x0, gint y0, gint x1, gint y1)
{
    GList *lp;
    
    if (x0 > x1) {
	gint t = x0;
	x0 = x1;
	x1 = t;
    }

    if (y0 > y1) {
	gint t = y0;
	y0 = y1;
	y1 = t;
    }
    
    unselect_all(nail_view);
    
    for (lp = nail_view->children; lp != NULL; lp = lp->next) {
	SivNailViewChild *child = lp->data;
	
	if (y0 <= child->y + NAIL_HEIGHT && child->y < y1) {
	    if (x0 <= child->x + NAIL_WIDTH && child->x < x1)
		select_add(nail_view, child);
	}
    }
}

static void invite_selection(SivNailView *nail_view)
{
    if (nail_view->selection != NULL) {
	SivNailViewChild *child = nail_view->selection->data;
	GtkWidget *widget = GTK_WIDGET(nail_view);
	gdouble adjval = nail_view->vadjustment->value;
	
	if (child->y + NAIL_HEIGHT > adjval + widget->allocation.height)
	    adjval = child->y + NAIL_HEIGHT - widget->allocation.height;
	
	if (child->y < adjval)
	    adjval = child->y;
	
	if (nail_view->vadjustment->value != adjval) {
	    nail_view->vadjustment->value = adjval;
	    gtk_adjustment_value_changed(nail_view->vadjustment);
	}
    }
}

static void siv_nail_view_view_prev(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	if (lp != NULL) {
	    if (lp->data == nail_view->last_view_nail) {
		lp = lp->prev;
		if (lp != NULL)
		    child = lp->data;
	    } else {
		child = lp->data;
	    }
	}
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[VIEW], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
	nail_view->last_view_nail = child;
    }
}

static void siv_nail_view_view_next(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	if (lp != NULL) {
	    if (lp->data == nail_view->last_view_nail) {
		lp = lp->next;
		if (lp != NULL)
		    child = lp->data;
	    } else {
		child = lp->data;
	    }
	}
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[VIEW], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
	nail_view->last_view_nail = child;
    }
}

static void siv_nail_view_sel_prev(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	if (lp != NULL) {
	    lp = lp->prev;
	    if (lp != NULL)
		child = lp->data;
	}
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[SELECT], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
    }
}

static void siv_nail_view_sel_next(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	if (lp != NULL) {
	    lp = lp->next;
	    if (lp != NULL)
		child = lp->data;
	}
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[SELECT], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
    }
}

static void siv_nail_view_sel_up(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	gint i;
	for (i = 0; lp != NULL && i < nail_view->ncols; i++)
	    lp = lp->prev;
	if (lp != NULL)
	    child = lp->data;
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[SELECT], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
    }
}

static void siv_nail_view_sel_down(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL) {
	GList *lp = g_list_find(nail_view->children, nail_view->selection->data);
	gint i;
	for (i = 0; lp != NULL && i < nail_view->ncols; i++)
	    lp = lp->next;
	if (lp != NULL)
	    child = lp->data;
    } else {
	if (nail_view->children != NULL)
	    child = nail_view->children->data;
    }
    
    if (child != NULL) {
	select_single(nail_view, child);
	invite_selection(nail_view);
	g_signal_emit(nail_view, signals[SELECT], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
    }
}

static void siv_nail_view_open_cur(GtkWidget *widget)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    SivNailViewChild *child = NULL;
    
    if (nail_view->selection != NULL)
	child = nail_view->selection->data;
    
    if (child != NULL) {
	g_signal_emit(nail_view, signals[OPEN], 0, siv_nail_get_path(SIV_NAIL(child->widget)));
    }
}

/* drag ½èÍý:
 *
 * press on sel'd icon
 *  motion -> dnd all
 *   release
 *  release -> click
 *
 * press on non-sel'd icon
 *  motion -> dnd single
 *   relase
 *  release -> sel
 *   
 * press on otherwhere
 *  motion -> sel all
 *   release
 *  release -> unsel
 */
    
static gboolean siv_nail_view_button_press_event(
	GtkWidget *widget, GdkEventButton *event)
{
    SivNailView *nail_view;
    gint x, y, dx, dy, ix, iy;
    
    printf("press\n");
    
    if (event->button != 1)
	return FALSE;
    
    nail_view = SIV_NAIL_VIEW(widget);
    nail_view->press_event = *event;
    
    x = event->x;
    y = event->y;
    nail_view->drag_begin_x = x;
    nail_view->drag_begin_y = y;
    
    nail_view->drag_dnd = FALSE;
    nail_view->drag_sel = FALSE;
    nail_view->press_child = NULL;
    
    if (x >= SPACING && y >= SPACING) {
	ix = (x - SPACING) / (NAIL_WIDTH + SPACING),
	dx = (x - SPACING) % (NAIL_WIDTH + SPACING);
	iy = (y - SPACING) / (NAIL_HEIGHT + SPACING),
	dy = (y - SPACING) % (NAIL_HEIGHT + SPACING);
	
	if (ix < nail_view->ncols && iy < nail_view->nrows) {
	    gint i = iy * nail_view->ncols + ix;
	    if (dx < NAIL_WIDTH && dy < NAIL_HEIGHT) {
		GList *lp = g_list_nth(nail_view->children, i);
		if (lp != NULL)
		    nail_view->press_child = lp->data;
	    }
	}
    }
    
    if (event->type == GDK_2BUTTON_PRESS) {
	printf("double-click\n");
	if (nail_view->press_child != NULL) {
	    g_signal_emit(nail_view, signals[OPEN], 0, siv_nail_get_path(SIV_NAIL(nail_view->press_child->widget)));
	    nail_view->last_view_nail = nail_view->press_child;
	}
    }
    
    return TRUE;
}

static gboolean siv_nail_view_motion_notify_event(
	GtkWidget *widget, GdkEventMotion *event)
{
    SivNailView *nail_view;
    gint x, y;
    
    printf("motion\n");
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    x = event->x;
    y = event->y;
    
    if (!nail_view->drag_dnd && !nail_view->drag_sel) {
	if (!gtk_drag_check_threshold(widget,
			nail_view->press_event.x,
			nail_view->press_event.y,
			event->x,
			event->y))
	    return TRUE;
	
	if (nail_view->press_child != NULL) {
	    if (g_list_find(nail_view->selection, nail_view->press_child) == NULL)
		select_single(nail_view, nail_view->press_child);
	    
	    gtk_drag_begin(widget,
		    nail_view->dnd_target_list,
		    GDK_ACTION_MOVE,
		    nail_view->press_event.button,
		    (GdkEvent *) event);
	    
	    nail_view->drag_dnd = TRUE;
	} else {
	    nail_view->drag_sel = TRUE;
	}
    }
    
    if (nail_view->drag_dnd) {
	
    } else if (nail_view->drag_sel) {
	select_multi(nail_view, nail_view->drag_begin_x, nail_view->drag_begin_y, x, y);
    }
    
    return TRUE;
}

static gboolean siv_nail_view_button_release_event(
	GtkWidget *widget, GdkEventButton *event)
{
    SivNailView *nail_view;
    
    printf("release\n");
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    if (!nail_view->drag_dnd && !nail_view->drag_sel) {
	if (nail_view->press_child != NULL)
	    select_single(nail_view, nail_view->press_child);
	else
	    unselect_all(nail_view);
    }
    
    return TRUE;
}

static gboolean siv_nail_view_scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    GtkAdjustment *adj = nail_view->vadjustment;
    gdouble val = adj->value;
    gdouble inc = adj->page_increment;
    
    switch (event->direction) {
    case GDK_SCROLL_UP:
	printf("scroll up\n");
	val -= inc;
	break;
	
    case GDK_SCROLL_DOWN:
	printf("scroll down\n");
	val += inc;
	break;
	
    default:
	;
    }
    
    val = CLAMP(val, adj->lower, adj->upper - adj->page_size);
    gtk_adjustment_set_value(adj, val);
    
    return TRUE;
}

static void siv_nail_view_drag_data_get(GtkWidget *widget,
	GdkDragContext *dc, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data)
{
    SivNailView *nail_view = SIV_NAIL_VIEW(widget);
    
    printf("drag_data_get\n");
    
    gtk_selection_data_set(
	    selection_data,
	    GDK_SELECTION_TYPE_STRING, 8,
	    "", 0);
}

static void siv_nail_view_drag_data_delete(GtkWidget *widget,
	GdkDragContext *dc, gpointer data)
{
    SivNailView *nail_view;
    
    nail_view = SIV_NAIL_VIEW(widget);
}

static gboolean siv_nail_view_drag_drop(GtkWidget *widget,
	GdkDragContext *dc, gint x, gint y,
	guint t, gpointer data)
{
    SivNailView *nail_view;
    GdkAtom atom;
    
    printf("drag_drop\n");
    
    nail_view = SIV_NAIL_VIEW(widget);
    atom = gtk_drag_dest_find_target(widget, dc, nail_view->dnd_target_list);
    gtk_drag_get_data(widget, dc, atom, t);
    
    printf("drag_drop done\n");
    
    return TRUE;
}

static void siv_nail_view_drag_data_received(GtkWidget *widget,
	GdkDragContext *dc, gint x, gint y,
	GtkSelectionData *selection_data,
	guint info, guint t, gpointer data)
{
    SivNailView *nail_view;
    GList *list, *lp;
    gboolean success;
    GtkWidget *src_widget;
    
    printf("drag_data_received\n");
    
    nail_view = SIV_NAIL_VIEW(widget);
    
    src_widget = gtk_drag_get_source_widget(dc);
    if (src_widget == NULL) {
	printf("different app.\n");
	gtk_drag_finish(dc, FALSE, FALSE, t);
	return;
    }
    if (src_widget == widget) {
	printf("same widget\n");
	gtk_drag_finish(dc, FALSE, FALSE, t);
	return;
    }
    if (!SIV_IS_NAIL_VIEW(src_widget)) {
	printf("not nail_view widget\n");
	gtk_drag_finish(dc, FALSE, FALSE, t);
	return;
    }
    
    list = SIV_NAIL_VIEW(src_widget)->selection;
    
    success = FALSE;
    
    if (list != NULL) {
	lp = list;
	while (lp != NULL) {
	    SivNailViewChild *child;
	    const gchar *oldpath, *p;
	    gchar newpath[PATH_MAX];
	    
	    child = lp->data;
	    lp = lp->next;
	    
	    oldpath = siv_nail_get_path(SIV_NAIL(child->widget));
	    p = strrchr(oldpath, '/');
	    if (p != NULL) {
		sprintf(newpath, "%s/%s", nail_view->dirpath, p + 1);
	    } else {
		sprintf(newpath, "%s/%s", nail_view->dirpath, oldpath);
	    }
	    
	    if (copy_file(oldpath, newpath)) {
		GtkWidget *icon = child->widget;
		g_object_ref(icon);
		gtk_container_remove(GTK_CONTAINER(src_widget), icon);
		gtk_container_add(GTK_CONTAINER(widget), icon);
		gtk_widget_set_state(child->widget, GTK_STATE_NORMAL);
		siv_nail_rename(SIV_NAIL(icon), newpath);
		gtk_widget_show(icon);
		g_object_unref(icon);
	    }
	}
	
	success = TRUE;
    }
    
    if (!success)
	gtk_drag_finish(dc, FALSE, FALSE, t);
    else
	gtk_drag_finish(dc, TRUE, TRUE, t);
}

static gboolean copy_file(const gchar *src, const gchar *dstdir)
{
    int pid;
    int st;
    
    printf("copy_file: src: %s, dstdir: %s\n", src, dstdir);
    switch (pid = fork()) {
    case 0:
	execlp("mv", "mv", src, dstdir, NULL);
	perror("mv");
	exit(1);
	break;
	
    case -1:
	perror("fork");
	return TRUE;
	break;
	
    default:
	while (waitpid(pid, &st, 0) == -1) {
	    if (errno != EINTR) {
		perror("waitpid");
		return FALSE;
	    }
	}
	if (!WIFEXITED(st))
	    return FALSE;
	if (WEXITSTATUS(st) != 0)
	    return FALSE;
	return TRUE;
    }
}
