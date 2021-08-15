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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rf.h"

/* RF sink interface */
int rf_write(rf_t *s, int16_t *iq_data, int samples)
{
	if(s->write)
	{
		return(s->write(s->private, iq_data, samples));
	}
	
	return(-1);
}

int rf_close(rf_t *s)
{
	if(s->close)
	{
		return(s->close(s->private));
	}
	
	return(0);
}

static double _hamming(double x)
{
	if(x < -1 || x > 1) return(0);
	return(0.54 - 0.46 * cos((M_PI * (1.0 + x))));
}

static double _rrc(double x, double b, double t)
{
	double r;
	
	/* Based on the Wikipedia page, https://en.wikipedia.org/w/index.php?title=Root-raised-cosine_filter&oldid=787851747 */
	
	if(x == 0)
	{
		r = (1.0 / t) * (1.0 + b * (4.0 / M_PI - 1));
	}
	else if(fabs(x) == t / (4.0 * b))
	{
		r = b / (t * sqrt(2.0)) * ((1.0 + 2.0 / M_PI) * sin(M_PI / (4.0 * b)) + (1.0 - 2.0 / M_PI) * cos(M_PI / (4.0 * b)));
	}
	else
	{
		double t1 = (4.0 * b * (x / t));
		double t2 = (sin(M_PI * (x / t) * (1.0 - b)) + 4.0 * b * (x / t) * cos(M_PI * (x / t) * (1.0 + b)));
		double t3 = (M_PI * (x / t) * (1.0 - t1 * t1));
		
		r = (1.0 / t) * (t2 / t3);
	}
	
	return(r);
}

void rf_qpsk_free(rf_qpsk_t *s)
{
	int i;
	for(i = 0; i < 4; i++)
	{
		free(s->taps[i]);
	}
	free(s->win);
}

int rf_qpsk_init(rf_qpsk_t *s, int interpolation, double level)
{
	const double sym[4][2] = { { -1, -1 }, { -1, 1 }, { 1, 1 }, { 1, -1 } };
	int i, x, n;
	double r, t;
	
	memset(s, 0, sizeof(rf_qpsk_t));
	
	/* Generate the symbol shape */
	s->interpolation = interpolation;
	s->ntaps = (10 * s->interpolation) | 1;
	
	for(i = 0; i < 4; i++)
	{
		s->taps[i] = malloc(sizeof(int16_t) * 2 * s->ntaps);
		if(!s->taps[i])
		{
			rf_qpsk_free(s);
			return(-1);
		}
		
		n = s->ntaps / 2;
		for(x = 0; x < s->ntaps; x++)
		{
			t = ((double) x - n) / s->interpolation;
			r = _rrc(t, 0.5, 1.0) * _hamming(((double) x - n) / n);
			s->taps[i][x * 2 + 0] = lround(r * sym[i][0] * M_SQRT1_2 * INT16_MAX * level);
			s->taps[i][x * 2 + 1] = lround(r * sym[i][1] * M_SQRT1_2 * INT16_MAX * level);
		}
	}
	
	/* Allocate memory for the output window */
	s->winx = 0;
	s->win = calloc(sizeof(int16_t) * 2, s->ntaps);
	if(!s->win)
	{
		rf_qpsk_free(s);
		return(-1);
	}
	
	/* Starting symbol */
	s->sym = 0;
	
	return(0);
}

int rf_qpsk_modulate(rf_qpsk_t *s, int16_t *dst, const uint8_t *src, int bits)
{
	const uint8_t map[4] = { 0, 3, 1, 2 };
	const int16_t *taps;
	int16_t *win;
	int x, i;
	
	for(x = 0; x < bits; x += 2)
	{
		/* Read out the next 2-bit symbol, MSB first */
		s->sym = (s->sym + map[(src[x >> 3] >> (6 - (x & 0x07))) & 0x03]) & 3;
		
		/* Update the output window with the new symbol */
		taps = s->taps[s->sym];
		
		win = &s->win[s->winx * 2];
		for(i = 0; i < (s->ntaps - s->winx); i++)
		{
			*(win++) += *(taps++);
			*(win++) += *(taps++);
		}
		
		win = &s->win[0];
		for(; i < s->ntaps; i++)
		{
			*(win++) += *(taps++);
			*(win++) += *(taps++);
		}
		
		for(i = 0; i < s->interpolation; i++)
		{
			*(dst++) = s->win[s->winx * 2 + 0];
			*(dst++) = s->win[s->winx * 2 + 1];
			
			s->win[s->winx * 2 + 0] = 0;
			s->win[s->winx * 2 + 1] = 0;
			
			if(++s->winx == s->ntaps) s->winx = 0;
		}
	}
	
	return(bits / 2 * s->interpolation);
}

