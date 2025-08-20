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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rf.h"

/* File sink */
typedef struct {
	FILE *f;
	void *data;
	size_t data_size;
	int samples;
	int type;
} rf_file_t;

static int _rf_file_write_uint8(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	uint8_t *u8 = rf->data;
	int i;
	
	while(samples)
	{
		for(i = 0; i < rf->samples && i < samples; i++, iq_data += 2)
		{
			u8[i * 2 + 0] = (iq_data[0] - INT16_MIN) >> 8;
			u8[i * 2 + 1] = (iq_data[1] - INT16_MIN) >> 8;
		}
		
		fwrite(rf->data, rf->data_size, i, rf->f);
		
		samples -= i;
	}
	
	return(0);
}

static int _rf_file_write_int8(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	int8_t *i8 = rf->data;
	int i;
	
	while(samples)
	{
		for(i = 0; i < rf->samples && i < samples; i++, iq_data += 2)
		{
			i8[i * 2 + 0] = iq_data[0] >> 8;
			i8[i * 2 + 1] = iq_data[1] >> 8;
		}
		
		fwrite(rf->data, rf->data_size, i, rf->f);
		
		samples -= i;
	}
	
	return(0);
}

static int _rf_file_write_uint16(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	uint16_t *u16 = rf->data;
	int i;
	
	while(samples)
	{
		for(i = 0; i < rf->samples && i < samples; i++, iq_data += 2)
		{
			u16[i * 2 + 0] = (iq_data[0] - INT16_MIN);
			u16[i * 2 + 1] = (iq_data[1] - INT16_MIN);
		}
		
		fwrite(rf->data, rf->data_size, i, rf->f);
		
		samples -= i;
	}
	
	return(0);
}

static int _rf_file_write_int16(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	
	fwrite(iq_data, sizeof(int16_t) * 2, samples, rf->f);
	
	return(0);
}

static int _rf_file_write_int32(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	int32_t *i32 = rf->data;
	int i;
	
	while(samples)
	{
		for(i = 0; i < rf->samples && i < samples; i++, iq_data += 2)
		{
			i32[i * 2 + 0] = (iq_data[0] << 16) + iq_data[0];
			i32[i * 2 + 1] = (iq_data[1] << 16) + iq_data[1];
		}
		
		fwrite(rf->data, rf->data_size, i, rf->f);
		
		samples -= i;
	}
	
	return(0);
}

static int _rf_file_write_float(void *private, int16_t *iq_data, int samples)
{
	rf_file_t *rf = private;
	float *f32 = rf->data;
	int i;
	
	while(samples)
	{
		for(i = 0; i < rf->samples && i < samples; i++, iq_data += 2)
		{
			f32[i * 2 + 0] = (float) iq_data[0] * (1.0 / 32767.0);
			f32[i * 2 + 1] = (float) iq_data[1] * (1.0 / 32767.0);
		}
		
		fwrite(rf->data, rf->data_size, i, rf->f);
		
		samples -= i;
	}
	
	return(0);
}

static int _rf_file_close(void *private)
{
	rf_file_t *rf = private;
	
	if(rf->f && rf->f != stdout) fclose(rf->f);
	if(rf->data) free(rf->data);
	free(rf);
	
	return(0);
}

int rf_file_open(rf_t *s, const char *filename, int type, int live)
{
	rf_file_t *rf = calloc(1, sizeof(rf_file_t));
	
	if(!rf)
	{
		perror("calloc");
		return(-1);
	}
	
	rf->type = type;
	
	if(filename == NULL)
	{
		fprintf(stderr, "No output filename provided.\n");
		_rf_file_close(rf);
		return(-1);
	}
	else if(strcmp(filename, "-") == 0)
	{
		rf->f = stdout;
	}
	else
	{
		rf->f = fopen(filename, "wb");	
		
		if(!rf->f)
		{
			perror("fopen");
			_rf_file_close(rf);
			return(-1);
		}
	}
	
	/* Find the size of the output data type */
	switch(type)
	{
	case RF_UINT8:  rf->data_size = sizeof(uint8_t);  break;
	case RF_INT8:   rf->data_size = sizeof(int8_t);   break;
	case RF_UINT16: rf->data_size = sizeof(uint16_t); break;
	case RF_INT16:  rf->data_size = sizeof(int16_t);  break;
	case RF_INT32:  rf->data_size = sizeof(int32_t);  break;
	case RF_FLOAT:  rf->data_size = sizeof(float);    break;
	default:
		fprintf(stderr, "%s: Unrecognised data type %d\n", __func__, type);
		_rf_file_close(rf);
		return(-1);
	}
	
	/* Double the size for complex types */
	rf->data_size *= 2;
	
	rf->samples = 1024;
	
	/* Allocate the memory, unless the output is int16 */
	if(rf->type != RF_INT16)
	{
		rf->data = malloc(rf->data_size * rf->samples);
		if(!rf->data)
		{
			perror("malloc");
			_rf_file_close(rf);
			return(-1);
		}
	}
	
	/* Register the callback functions */
	s->private = rf;
	s->close = _rf_file_close;
	
	switch(type)
	{
	case RF_UINT8:  s->write = _rf_file_write_uint8;  break;
	case RF_INT8:   s->write = _rf_file_write_int8;   break;
	case RF_UINT16: s->write = _rf_file_write_uint16; break;
	case RF_INT16:  s->write = _rf_file_write_int16;  break;
	case RF_INT32:  s->write = _rf_file_write_int32;  break;
	case RF_FLOAT:  s->write = _rf_file_write_float;  break;
	}
	
	/* Is this a live target? */
	s->live = live ? 1 : 0;
	
	return(0);
}

