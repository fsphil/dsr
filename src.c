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

#include <stddef.h>
#include "src.h"

int src_read_mono(src_t *s, int16_t *dst, int step, int samples)
{
	int i;
	
	if(!s || s->audio_len < 0)
	{
		/* EOF */
		return(0);
	}
	
	for(i = 0; i < samples;)
	{
		/* Fetch a new buffer if no audio is available */
		if(s->audio_len == 0)
		{
			if(!s->read) s->audio_len = -1;
			else s->audio_len = s->read(s->private, s->audio, s->audio_step);
		}
		
		/* Test for EOF */
		if(s->audio_len < 0)
		{
			s->eof = -1;
			break;
		}
		
		/* Copy audio */
		for(; s->audio_len > 0 && i < samples; i++)
		{
			*dst = (*s->audio[0] + *s->audio[1]) / 2;
			dst += step;
			
			s->audio[0] += s->audio_step[0];
			s->audio[1] += s->audio_step[1];
			s->audio_len--;
		}
	}
	
	return(i);
}

int src_read_stereo(src_t *s, int16_t *dst_l, int step_l, int16_t *dst_r, int step_r, int samples)
{
	int i;
	
	if(!s || s->audio_len < 0)
	{
		/* EOF */
		return(0);
	}
	
	for(i = 0; i < samples;)
	{
		/* Fetch a new buffer if no audio is available */
		if(s->audio_len == 0)
		{
			if(!s->read) s->audio_len = -1;
			else s->audio_len = s->read(s->private, s->audio, s->audio_step);
		}
		
		/* Test for EOF */
		if(s->audio_len < 0)
		{
			s->eof = -1;
			break;
		}
		
		/* Copy audio */
		for(; s->audio_len > 0 && i < samples; i++)
		{
			*dst_l = *s->audio[0];
			dst_l += step_l;
			s->audio[0] += s->audio_step[0];
			
			*dst_r = *s->audio[1];
			dst_r += step_r;
			s->audio[1] += s->audio_step[1];
			
			s->audio_len--;
		}
	}
	
	return(i);
}

int src_eof(src_t *s)
{
	if(!s) return(-1); /* EOF */
	return(s->eof);
}

int src_close(src_t *s)
{
	if(!s || !s->close) return(0);
	return(s->close(s->private));
}

