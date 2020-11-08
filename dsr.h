/* dsr - Digitale Satelliten Radio (DSR) encoder                         */
/*=======================================================================*/
/* Copyright 2020 Philip Heron <phil@sanslogic.co.uk>                    */
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

#ifndef _DSR_H
#define _DSR_H

#include <stdint.h>

#define DSR_SAMPLE_RATE 32000
#define DSR_SYMBOL_RATE 10240000

typedef struct {
	int16_t *samples;
	int step;
} dsr_audio_block_t;

typedef struct {
	int type;
	int music;
	int mode;
	char name[9];
} dsr_channel_t;

typedef struct {
	
	dsr_channel_t channels[32];
	
	int frame;
	uint8_t sa[128][8];
	int16_t delay[8192];
	
} dsr_t;

extern void dsr_frames(dsr_t *s, uint8_t *a, uint8_t *b);
extern void dsr_encode(dsr_t *s, uint8_t *block, const dsr_audio_block_t *audio);
extern void dsr_init(dsr_t *s);

#endif

