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
 * $Id: image.c 137 2005-03-23 16:35:13Z masm $
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#ifdef DECODER_PROCESS
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#endif
#include "image-format.h"
#include "image.h"

#ifdef DECODER_PROCESS

static int child_pid = -1;
static int pipe_fd = -1;
static int shm_fd = -1;

static int pipe_wait(void)
{
    while (1) {
	char c;
	switch (read(pipe_fd, &c, 1)) {
	case -1:
	    perror("read");
	    continue;
	case 0:
	    return 0;
	}
	return 1;
    }
}

static void pipe_signal(void)
{
    char c = 0;
    write(pipe_fd, &c, 1);
}

static void image_read_sub(void)
{
    while (1) {
	unsigned char *shm_map;
	unsigned long size;
	struct siv_image_t *img;
	
	if (!pipe_wait())
	    exit(1);
	
	shm_map = mmap(NULL, 4, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	size = *(unsigned long *) shm_map;
	munmap(shm_map, 4);
	
	shm_map = mmap(NULL, 4 + size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	img = image_format_read(shm_map + 4, size);
	munmap(shm_map, 4 + size);
	
	if (img == NULL) {
	    ftruncate(shm_fd, 4);
	    shm_map = mmap(NULL, 4, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	    *(unsigned int *) shm_map = 0;
	    munmap(shm_map, 4);
	} else {
	    int bpp;
	    unsigned long mapsiz;
	    unsigned char *p;
	    
	    bpp = img->has_alpha ? 4 : 3;
	    
	    mapsiz = 4 + 4 + 4 + 4;
	    mapsiz += img->width * img->height * bpp;
	    if (img->comment != NULL)
		mapsiz += strlen(img->comment);
	    mapsiz++;
	    if (img->short_info != NULL)
		mapsiz += strlen(img->short_info);
	    mapsiz++;
	    if (img->long_info != NULL)
		mapsiz += strlen(img->long_info);
	    mapsiz++;
	    
	    ftruncate(shm_fd, mapsiz);
	    shm_map = mmap(NULL, mapsiz, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	    p = (unsigned char *) shm_map;
	    
	    *(unsigned long *) p = mapsiz;
	    p += sizeof(unsigned long);
	    
	    *(unsigned long *) p = img->has_alpha;
	    p += sizeof(unsigned long);
	    
	    *(unsigned long *) p = img->width;
	    p += sizeof(unsigned long);
	    *(unsigned long *) p = img->height;
	    p += sizeof(unsigned long);
	    
	    memcpy(p, img->data, img->width * img->height * bpp);
	    p += img->width * img->height * bpp;
	    
	    if (img->comment != NULL) {
		strcpy(p, img->comment);
		p += strlen(p) + 1;
	    } else
		*p++ = '\0';
	    
	    if (img->short_info != NULL) {
		strcpy(p, img->short_info);
		p += strlen(p) + 1;
	    } else
		*p++ = '\0';
	    
	    if (img->long_info != NULL) {
		strcpy(p, img->long_info);
		p += strlen(p) + 1;
	    } else
		*p++ = '\0';
	    
	    munmap(shm_map, mapsiz);
	    image_destroy(img);
	}
	
	pipe_signal();
    }
}

static char *make_shm_name(void)
{
    static char name[1024];
    sprintf(name, "/siv.%d", getpid());
    return name;
}

static void signal_handler(int sig)
{
    int st;
    if (wait4(child_pid, &st, WNOHANG, NULL) > 0)
	child_pid = -1;
}

static void create_sub_proc(void)
{
    int fds[2];
    
    signal(SIGCHLD, signal_handler);
    
    if (pipe_fd != -1) {
	close(pipe_fd);
	pipe_fd = -1;
    }
    
    if (shm_fd != -1) {
	close(shm_fd);
	shm_fd = -1;
    }
    
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
	perror("socketpair");
	exit(1);
    }
    
    shm_fd = shm_open(make_shm_name(), O_RDWR|O_CREAT, 0);
    if (shm_fd == -1) {
	perror(make_shm_name());
	exit(1);
    }
    shm_unlink(make_shm_name());
    
    child_pid = fork();
    
    if (child_pid == -1) {
	perror("fork");
	exit(1);
    }
    
    if (child_pid == 0) {
	close(fds[0]);
	pipe_fd = fds[1];
	image_read_sub();
	exit(1);
    }
    
    close(fds[1]);
    pipe_fd = fds[0];
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static struct siv_image_t *image_read(unsigned char *data, unsigned long size)
{
    unsigned char *shm_map, *p;
    unsigned long mapsiz;
    struct siv_image_t *img;
    
    pthread_mutex_lock(&mutex);
    
    if (child_pid == -1) {
	create_sub_proc();
    }
    
    ftruncate(shm_fd, 4 + size);
    shm_map = mmap(NULL, 4 + size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *(unsigned long *) shm_map = size;
    memcpy(shm_map + 4, data, size);
    munmap(shm_map, 4 + size);
    
    pipe_signal();
    
    if (!pipe_wait())
	exit(1);
    
    shm_map = mmap(NULL, 4, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    p = shm_map;
    mapsiz = *(unsigned long *) p;
    p += sizeof(unsigned long);
    munmap(shm_map, 4);
    
    if (mapsiz == 0) {
	img = NULL;
    } else {
	shm_map = mmap(NULL, mapsiz, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	p = shm_map + 4;
	
	img = malloc(sizeof *img);
	
	img->has_alpha = *(unsigned long *) p;
	p += sizeof(unsigned long);
	
	img->width = *(unsigned long *) p;
	p += sizeof(unsigned long);
	img->height = *(unsigned long *) p;
	p += sizeof(unsigned long);
	
	img->data = malloc(img->width * img->height * (img->has_alpha ? 4 : 3));
	memcpy(img->data, p, img->width * img->height * (img->has_alpha ? 4 : 3));
	
	if (*p != '\0') {
	    img->comment = malloc(strlen(p) + 1);
	    strcpy(img->comment, p);
	} else {
	    img->comment = NULL;
	    p++;
	}
	
	if (*p != '\0') {
	    img->short_info = malloc(strlen(p) + 1);
	    strcpy(img->short_info, p);
	} else {
	    img->short_info = NULL;
	    p++;
	}
	
	if (*p != '\0') {
	    img->long_info = malloc(strlen(p) + 1);
	    strcpy(img->long_info, p);
	} else {
	    img->long_info = NULL;
	    p++;
	}
	
	munmap(shm_map, mapsiz);
    }
    
    pthread_mutex_unlock(&mutex);
    
    return img;
}

#else

static struct siv_image_t *image_read(unsigned char *data, unsigned long size)
{
    struct siv_image_t *img;
    
    img = image_format_read(data, size);
    
    return img;
}

#endif

struct siv_image_t *image_read_file(const char *fname)
{
    int fd = -1;
    unsigned long size = 0;
    struct stat st;
    unsigned char *map = NULL;
    struct siv_image_t *img = NULL;
    
    if ((fd = open(fname, O_RDONLY)) == -1) {
	perror(fname);
	goto error;
    }
    
    if (fstat(fd, &st) == -1) {
	perror(fname);
	goto error;
    }
    
    if (!S_ISREG(st.st_mode)) {
	fprintf(stderr, "%s: not a regular file.\n", fname);
	goto error;
    }
    
    size = st.st_size;
    
    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == NULL) {
	perror(fname);
	goto error;
    }
    
    img = image_read(map, size);
    
    close(fd);
    munmap(map, size);
    
    return img;
    
 error:
    if (fd != -1)
	close(fd);
    if (map != NULL)
	munmap(map, size);
    if (img != NULL)
	image_destroy(img);
    return NULL;
}

void image_destroy(struct siv_image_t *img)
{
    if (img->data != NULL)
	free(img->data);
    if (img->comment != NULL)
	free(img->comment);
    if (img->short_info != NULL)
	free(img->short_info);
    if (img->long_info != NULL)
	free(img->long_info);
    free(img);
}

/*EOF*/
