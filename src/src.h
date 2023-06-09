/* dsr - Digitale Satelliten Radio (DSR) encoder                         */
/*=======================================================================*/
/* Copyright 2021 Philip Heron <phil@sanslogic.co.uk>                    */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdint.h>

#ifndef _SRC_H
#define _SRC_H

#define SRC_SAMPLE_RATE 32000

typedef int (*src_read_t)(void *private, int16_t *audio[2], int audio_step[2]);
typedef int (*src_close_t)(void *private);

typedef struct {
	
	src_read_t read;
	src_close_t close;
	void *private;
	
	int16_t *audio[2];
	int audio_step[2];
	int audio_len;
	int eof;
	
} src_t;

extern int src_read_stereo(src_t *s, int16_t *dst_l, int step_l, int16_t *dst_r, int step_r, int samples);
extern int src_read_mono(src_t *s, int16_t *dst, int step, int samples);
extern int src_eof(src_t *s);
extern int src_close(src_t *s);

#include "src_tone.h"
#include "src_rawaudio.h"
#ifdef HAVE_FFMPEG
#include "src_ffmpeg.h"
#endif

#endif

