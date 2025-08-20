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

#ifndef _RF_H
#define _RF_H

/* File output types */
#define RF_UINT8  0
#define RF_INT8   1
#define RF_UINT16 2
#define RF_INT16  3
#define RF_INT32  4
#define RF_FLOAT  5 /* 32-bit float */

/* Callback prototypes */
typedef int (*rf_write_t)(void *private, int16_t *iq_data, int samples);
typedef int (*rf_close_t)(void *private);

typedef struct {
	
	void *private;
	rf_write_t write;
	rf_close_t close;
	
	double scale;
	int live;
	
} rf_t;

extern double rf_scale(rf_t *s);
extern int rf_write(rf_t *s, int16_t *iq_data, int samples);
extern int rf_close(rf_t *s);

typedef struct {
	
	int interpolation;
	int ntaps;
	int16_t *taps[4];
	
	/* Output window */
	int winx;
	int16_t *win;
	
	/* Differential state */
	int sym;
	
} rf_qpsk_t;

extern void rf_qpsk_free(rf_qpsk_t *s);
extern int rf_qpsk_init(rf_qpsk_t *s, int interpolation, double level);
extern int rf_qpsk_modulate(rf_qpsk_t *s, int16_t *dst, const uint8_t *src, int bits);

#include "rf_file.h"
#include "rf_hackrf.h"

#ifdef HAVE_SOAPYSDR
#include "rf_soapysdr.h"
#endif

#endif

