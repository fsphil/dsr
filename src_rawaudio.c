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
#include <stdio.h>
#include <stdlib.h>
#include "src.h"

typedef struct {
	
	int16_t *audio;
	int audio_len;
	
	FILE *f;
	int exec;
	int channels;
	int repeat;
	
} src_rawaudio_t;

static int _src_rawaudio_read(src_rawaudio_t *src, int16_t *audio[2], int audio_step[2])
{
	int i;
	
	if(feof(src->f))
	{
		/* EOF -- rewind to beginning or signal EOF */
		if(src->repeat) fseek(src->f, 0, SEEK_SET);
		else return(-1);
	}
	
	i = fread(src->audio, sizeof(int16_t) * src->channels, src->audio_len, src->f);
	
	if(src->channels == 1)
	{
		/* Mono, mapped to two stereo tracks */
		audio[0] = audio[1] = src->audio;
		audio_step[0] = audio_step[1] = 1;
	}
	else
	{
		/* Stereo */
		audio[0] = src->audio + 0;
		audio[1] = src->audio + 1;
		audio_step[0] = audio_step[1] = 2;
	}
	
	return(i);
}

static int _src_rawaudio_close(src_rawaudio_t *src)
{
	if(src->exec) pclose(src->f);
	else fclose(src->f);
	free(src->audio);
	free(src);
	return(0);
}

int src_rawaudio_open(src_t *s, const char *filename, int exec, int stereo, int repeat)
{
	src_rawaudio_t *src;
	
	src = calloc(1, sizeof(src_rawaudio_t));
	if(!src)
	{
		return(-1);
	}
	
	src->exec = exec;
	src->f = (src->exec ? popen(filename, "r") : fopen(filename, "rb"));
	if(!src->f)
	{
		perror(filename);
		free(src);
		return(-1);
	}
	src->channels = stereo ? 2 : 1;
	src->repeat = repeat;
	
	/* Allocate memory for output buffer (0.1 seconds) */
	src->audio_len = SRC_SAMPLE_RATE * 0.1;
	src->audio = malloc(src->audio_len * sizeof(int16_t) * src->channels);
	if(!src->audio)
	{
		if(src->exec) pclose(src->f);
		else fclose(src->f);
		free(src);
		return(-1);
	}
	
	/* Register the callback functions */
	s->private = src;
	s->read = (src_read_t) _src_rawaudio_read;
	s->close = (src_close_t) _src_rawaudio_close;
	
	return(0);
}

