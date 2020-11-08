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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dsr.h"

int main(int argc, char *argv[])
{
	const float syms[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
	const int symmap[4] = { 0, 3, 1, 2 };
	uint8_t block[5120];
	float output[40960];
	int i, s, l, d;
	dsr_t dsr;
	dsr_audio_block_t audio[32];
	FILE *fin[32];
	FILE *f = fopen("frame.bin", "wb");
	
	/* Allocate memory for the audio channels */
	for(i = 0; i < 32; i++)
	{
		char fname[256];
		
		snprintf(fname, 256, "audio/channel%02d.raw", i + 1);
		fin[i] = fopen(fname, "rb");
		if(fin[i])
		{
			audio[i].samples = calloc(sizeof(int16_t), 64);
			audio[i].step = 1;
		}
		else
		{
			audio[i].samples = NULL;
			audio[i].step = 0;
		}
	}
	
	dsr_init(&dsr);
	
	d = 0;
	
	for(i = 0; i < 500 * 60; i++) /* 60 seconds */
	{
		for(l = 0; l < 32; l++)
		{
			if(audio[l].samples)
			{
				fread(audio[l].samples, sizeof(int16_t), 64, fin[l]);
				if(feof(fin[l]))
				{
					fclose(fin[l]);
					free(audio[l].samples);
					audio[l].samples = NULL;
					audio[l].step = 0;
				}
			}
		}
		
		/* Encode the next audio block (2ms) */
		dsr_encode(&dsr, block, audio);
		
		/* Generate the output symbols */
		for(l = 0; l < 40960; l += 2)
		{
			s = (block[l >> 3] >> (6 - (l & 7))) & 3;
			d = (d + symmap[s]) & 3;
			output[l + 0] = syms[d][0];
			output[l + 1] = syms[d][1];
		}
		
		fwrite(output, sizeof(float), 40960, f);
	}
	
	fclose(f);
	
	for(i = 0; i < 32; i++)
	{
		if(audio[i].samples)
		{
			fclose(fin[i]);
			free(audio[i].samples);
		}
	}
	
	return(0);
}

