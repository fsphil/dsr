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

#include <math.h>
#include <stdlib.h>
#include "src.h"

typedef struct {
	
	int16_t *audio;
	int audio_len;
	
	double x;
	double delta;
	double level;
	
} src_tone_t;

static int _src_tone_read(src_tone_t *src, int16_t *audio[2], int audio_step[2])
{
	int i;
	
	/* Render the tone */
	for(i = 0; i < src->audio_len; i++)
	{
		src->audio[i] = sin(src->x) * src->level * INT16_MAX;
		src->x += src->delta;
	}
	
	/* Map our mono signal to the two stereo track */
	audio[0] = audio[1] = src->audio;
	audio_step[0] = audio_step[1] = 1;
	
	return(src->audio_len);
}

static int _src_tone_close(src_tone_t *src)
{
	free(src->audio);
	free(src);
	return(0);
}

int src_tone_open(src_t *s, double frequency, double level)
{
	src_tone_t *src;
	
	src = calloc(1, sizeof(src_tone_t));
	if(!src)
	{
		return(-1);
	}
	
	/* Allocate memory for output buffer (1 second) */
	src->audio_len = SRC_SAMPLE_RATE;
	src->audio = malloc(src->audio_len * sizeof(int16_t));
	if(!src->audio)
	{
		free(src);
		return(-1);
	}
	
	/* Configure the sine wave */	
	src->x = 0;
	src->delta = 2.0 * M_PI * frequency / SRC_SAMPLE_RATE;
	src->level = level;
	
	/* Register the callback functions */
	s->private = src;
	s->read = (src_read_t) _src_tone_read;
	s->close = (src_close_t) _src_tone_close;
	
	return(0);
}

