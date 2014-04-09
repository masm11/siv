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
 * $Id: image-format.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include "image-format.h"

#define MAX_FORMATS 32

static struct {
    struct siv_image_format_t *fmt;
    void *dl;
} format[MAX_FORMATS] = { { 0, }, };

static void add_format(void *dl, struct siv_image_format_t *fmt)
{
    int i;
    printf("add_format: %p, %s, %p\n", dl, fmt->format_name, fmt->read);
    for (i = 0; i < MAX_FORMATS; i++) {
	if (format[i].dl == NULL) {
	    format[i].dl = dl;
	    format[i].fmt = fmt;
	    return;
	}
    }
}

static void handle_so(char *path)
{
    void *dl;
    struct siv_image_format_t *fmt;
    
    dl = dlopen(path, RTLD_NOW);
    if (dl == NULL) {
	fprintf(stderr, "%s: %s\n", path, dlerror());
	return;
    }
    fmt = dlsym(dl, "format");
    if (fmt == NULL) {
	dlclose(dl);
	return;
    }
    add_format(dl, fmt);
}

static void handle_la(char *path)
{
    FILE *fp;
    char dlpath[PATH_MAX], *dlname;
    
    strcpy(dlpath, path);
    if ((dlname = strrchr(dlpath, '/')) == NULL)
	dlname = dlpath;
    else
	dlname++;
    
    if ((fp = fopen(path, "r")) == NULL) {
	perror(path);
	return;
    }
    
    while (1) {
	char buf[128];
	if (fgets(buf, sizeof buf, fp) == NULL)
	    break;
	
	if (sscanf(buf, "dlname='%[^']'", dlname) == 1) {
	    handle_so(dlpath);
	    break;
	}
    }
}

static void find_dll_in_dir(char *dirpath)
{
    DIR *dir;
    struct dirent *dp;
    
    dir = opendir(dirpath);
    if (dir == NULL) {
	if (errno != ENOENT)
	    perror(dirpath);
	return;
    }
    
    while ((dp = readdir(dir)) != NULL) {
	char path[PATH_MAX];
	int namelen;
	
	namelen = strlen(dp->d_name);
	if (namelen < 3)
	    continue;
	
	if (strcmp(&dp->d_name[namelen - 3], ".la") != 0)
	    continue;
	
	sprintf(path, "%s/%s", dirpath, dp->d_name);
	handle_la(path);
    }
}

void image_format_init(void)
{
    char path[PATH_MAX];
    char *home;
    
    home = getenv("HOME");
    if (home != NULL) {
	sprintf(path, "%s/%s", home, ".siv");
	find_dll_in_dir(path);
    }
    
    sprintf(path, "%s", PKGLIBEXECDIR);
    find_dll_in_dir(path);
}

struct siv_image_t *image_format_read(unsigned char *data, unsigned long size)
{
    int i;
    
    for (i = 0; i < MAX_FORMATS; i++) {
	if (format[i].fmt != NULL) {
	    struct siv_image_t *img;
	    img = (format[i].fmt->read)(data, size);
	    if (img != NULL)
		return img;
	}
    }
    
    return NULL;
}

/*EOF*/
