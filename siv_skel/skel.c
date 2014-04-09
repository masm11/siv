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
 * $Id: skel.c 137 2005-03-23 16:35:13Z masm $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <siv/image.h>
#include <siv/format.h>

static struct siv_image_t *skel_read(unsigned char *data, unsigned long size)
{
#if 0
    /* ���δؿ��ϡ�������Ϳ����줿�ǡ������ᤷ��
     * ������������롣
     * �����Ǥ����� struct siv_image_t * �� malloc() �����֤���
     * �����Ǥ��ʤ���� NULL ���֤���
     */
    
    struct siv_image_t *img;
    
    img = malloc(sizeof *img);
    img->has_alpha = 0;		// data �� alpha ��¸�ߤ��뤫�ɤ�����
    img->width = 0;		// ��������
    img->height = 0;		// �����ι⤵
    
    /* �����ǡ���
     * �������夫�鱦�ء�
     * R, G, B, A ���줾�� 1�Х��Ȥ� 0��255��
     * alpha �ʤ��ξ��� R, G, B, R, G, B, ...
     * alpha ����ξ��� R, G, B, A, R, G, B, A, ...
     * A �� 0 ��Ʃ��, 255 ����Ʃ����
     */
    img->data = malloc(img->width * img->height * 3);
    
    /* ���ΤȤ���ȤäƤʤ��Τǡ�NULL �ˤ��Ƥ�����
     */
    img->comment = NULL;
    img->short_info = NULL;
    img->long_info = NULL;
    
    if (failed) {
	/* ���Ԥ�����硢image_destroy() �� img ��������Ƥ�����
	 * ���δؿ��ϡ�
	 *   img->data
	 *   img->comment
	 *   img->short_info
	 *   img->long_info
	 * �ˤĤ��Ƥ⡢NULL �Ǥʤ���в������롣
	 */
	image_destroy(img);
	img = NULL;
    }
    
    return img;
#endif
    
    return NULL;
}

struct siv_image_format_t format = {
    "SKEL",
    skel_read,
};
