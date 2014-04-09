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
 * $Id: sivnailview.h 137 2005-03-23 16:35:13Z masm $
 */
#ifndef SIV_NAIL_VIEW_H__INCLUDED
#define SIV_NAIL_VIEW_H__INCLUDED

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkdnd.h>

#define SIV_TYPE_NAIL_VIEW            (siv_nail_view_get_type ())
#define SIV_NAIL_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SIV_TYPE_NAIL_VIEW, SivNailView))
#define SIV_NAIL_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SIV_TYPE_NAIL_VIEW, SivNailViewClass))
#define SIV_IS_NAIL_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SIV_TYPE_NAIL_VIEW))
#define SIV_IS_NAIL_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SIV_TYPE_NAIL_VIEW))
#define SIV_NAIL_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SIV_TYPE_NAIL_VIEW, SivNailViewClass))

typedef struct _SivNailView        SivNailView;
typedef struct _SivNailViewClass   SivNailViewClass;

struct _SivNailView {
    GtkContainer container;
    
    gchar *dirpath;
    
    GList *children;
    
    guint ncols, nrows;
    
    guint width;
    guint height;
    
    GtkAdjustment *vadjustment;
    
    GdkWindow *subwin;
    
    guint freeze_count;
    
    GList *selection;
    struct _SivNailViewChild *last_view_nail;
    
    GtkTargetList *dnd_target_list;
    gint drag_begin_x, drag_begin_y;
    GdkEventButton press_event;
    gboolean drag_dnd, drag_sel;
    struct _SivNailViewChild *press_child;
};

struct _SivNailViewClass {
    GtkContainerClass parent_class;
    
    void (*set_scroll_adjustments) (
	    SivNailView *layout,
	    GtkAdjustment *vadjustment);
    
    void (*view_prev)(GtkWidget *widget);
    void (*view_next)(GtkWidget *widget);
    void (*sel_prev)(GtkWidget *widget);
    void (*sel_next)(GtkWidget *widget);
    void (*sel_up)(GtkWidget *widget);
    void (*sel_down)(GtkWidget *widget);
    void (*open_cur)(GtkWidget *widget);
};

GType          siv_nail_view_get_type        (void) G_GNUC_CONST;
GtkWidget*     siv_nail_view_new             (const gchar *dir,
					      GtkAdjustment *vadjustment);
void           siv_nail_view_put             (SivNailView     *layout, 
					      GtkWidget     *child_widget);

GtkAdjustment* siv_nail_view_get_vadjustment (SivNailView     *layout);
void           siv_nail_view_set_vadjustment (SivNailView     *layout,
					      GtkAdjustment *adjustment);

void           siv_nail_view_freeze          (SivNailView     *layout);
void           siv_nail_view_thaw            (SivNailView     *layout);

#endif	/* ifndef SIV_NAIL_VIEW_H__INCLUDED */
