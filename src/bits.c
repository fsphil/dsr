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

#include <stdint.h>
#include "bits.h"

int bits_write_uint(uint8_t *b, int x, uint64_t bits, int nbits)
{
	uint64_t m = UINT64_MAX >> (64 - nbits);
	int s;
	
	/* Zero unwanted bits */
	bits &= m;
	
	/* Move pointer ahead to first affected byte */
	b += x >> 3;
	
	for(s = nbits - (8 - (x & 7)); s >= 0; s -= 8, b++)
	{
		*b &= ~(m >> s);
		*b |= bits >> s;
	}
	
	if(s < 0)
	{
		s = -s;
		*b &= ~(m << s);
		*b |= bits << s;
	}
	
	return(x + nbits);
}

int bits_write_int(uint8_t *b, int x, int64_t bits, int nbits)
{
	return(bits_write_uint(b, x, (uint64_t) bits, nbits));
}

