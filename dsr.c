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
#include <string.h>
#include <stdio.h>
#include "bits.h"
#include "dsr.h"

static const uint16_t _ileave[256] = {
	0x0000,0x0001,0x0004,0x0005,0x0010,0x0011,0x0014,0x0015,
	0x0040,0x0041,0x0044,0x0045,0x0050,0x0051,0x0054,0x0055,
	0x0100,0x0101,0x0104,0x0105,0x0110,0x0111,0x0114,0x0115,
	0x0140,0x0141,0x0144,0x0145,0x0150,0x0151,0x0154,0x0155,
	0x0400,0x0401,0x0404,0x0405,0x0410,0x0411,0x0414,0x0415,
	0x0440,0x0441,0x0444,0x0445,0x0450,0x0451,0x0454,0x0455,
	0x0500,0x0501,0x0504,0x0505,0x0510,0x0511,0x0514,0x0515,
	0x0540,0x0541,0x0544,0x0545,0x0550,0x0551,0x0554,0x0555,
	0x1000,0x1001,0x1004,0x1005,0x1010,0x1011,0x1014,0x1015,
	0x1040,0x1041,0x1044,0x1045,0x1050,0x1051,0x1054,0x1055,
	0x1100,0x1101,0x1104,0x1105,0x1110,0x1111,0x1114,0x1115,
	0x1140,0x1141,0x1144,0x1145,0x1150,0x1151,0x1154,0x1155,
	0x1400,0x1401,0x1404,0x1405,0x1410,0x1411,0x1414,0x1415,
	0x1440,0x1441,0x1444,0x1445,0x1450,0x1451,0x1454,0x1455,
	0x1500,0x1501,0x1504,0x1505,0x1510,0x1511,0x1514,0x1515,
	0x1540,0x1541,0x1544,0x1545,0x1550,0x1551,0x1554,0x1555,
	0x4000,0x4001,0x4004,0x4005,0x4010,0x4011,0x4014,0x4015,
	0x4040,0x4041,0x4044,0x4045,0x4050,0x4051,0x4054,0x4055,
	0x4100,0x4101,0x4104,0x4105,0x4110,0x4111,0x4114,0x4115,
	0x4140,0x4141,0x4144,0x4145,0x4150,0x4151,0x4154,0x4155,
	0x4400,0x4401,0x4404,0x4405,0x4410,0x4411,0x4414,0x4415,
	0x4440,0x4441,0x4444,0x4445,0x4450,0x4451,0x4454,0x4455,
	0x4500,0x4501,0x4504,0x4505,0x4510,0x4511,0x4514,0x4515,
	0x4540,0x4541,0x4544,0x4545,0x4550,0x4551,0x4554,0x4555,
	0x5000,0x5001,0x5004,0x5005,0x5010,0x5011,0x5014,0x5015,
	0x5040,0x5041,0x5044,0x5045,0x5050,0x5051,0x5054,0x5055,
	0x5100,0x5101,0x5104,0x5105,0x5110,0x5111,0x5114,0x5115,
	0x5140,0x5141,0x5144,0x5145,0x5150,0x5151,0x5154,0x5155,
	0x5400,0x5401,0x5404,0x5405,0x5410,0x5411,0x5414,0x5415,
	0x5440,0x5441,0x5444,0x5445,0x5450,0x5451,0x5454,0x5455,
	0x5500,0x5501,0x5504,0x5505,0x5510,0x5511,0x5514,0x5515,
	0x5540,0x5541,0x5544,0x5545,0x5550,0x5551,0x5554,0x5555,
};

static const uint8_t _par[256] = {
	0x00,0x00,0x03,0x03,0x05,0x05,0x06,0x06,0x09,0x09,0x0A,0x0A,0x0C,0x0C,0x0F,0x0F,
	0x11,0x11,0x12,0x12,0x14,0x14,0x17,0x17,0x18,0x18,0x1B,0x1B,0x1D,0x1D,0x1E,0x1E,
	0x21,0x21,0x22,0x22,0x24,0x24,0x27,0x27,0x28,0x28,0x2B,0x2B,0x2D,0x2D,0x2E,0x2E,
	0x30,0x30,0x33,0x33,0x35,0x35,0x36,0x36,0x39,0x39,0x3A,0x3A,0x3C,0x3C,0x3F,0x3F,
	0x41,0x41,0x42,0x42,0x44,0x44,0x47,0x47,0x48,0x48,0x4B,0x4B,0x4D,0x4D,0x4E,0x4E,
	0x50,0x50,0x53,0x53,0x55,0x55,0x56,0x56,0x59,0x59,0x5A,0x5A,0x5C,0x5C,0x5F,0x5F,
	0x60,0x60,0x63,0x63,0x65,0x65,0x66,0x66,0x69,0x69,0x6A,0x6A,0x6C,0x6C,0x6F,0x6F,
	0x71,0x71,0x72,0x72,0x74,0x74,0x77,0x77,0x78,0x78,0x7B,0x7B,0x7D,0x7D,0x7E,0x7E,
	0x81,0x81,0x82,0x82,0x84,0x84,0x87,0x87,0x88,0x88,0x8B,0x8B,0x8D,0x8D,0x8E,0x8E,
	0x90,0x90,0x93,0x93,0x95,0x95,0x96,0x96,0x99,0x99,0x9A,0x9A,0x9C,0x9C,0x9F,0x9F,
	0xA0,0xA0,0xA3,0xA3,0xA5,0xA5,0xA6,0xA6,0xA9,0xA9,0xAA,0xAA,0xAC,0xAC,0xAF,0xAF,
	0xB1,0xB1,0xB2,0xB2,0xB4,0xB4,0xB7,0xB7,0xB8,0xB8,0xBB,0xBB,0xBD,0xBD,0xBE,0xBE,
	0xC0,0xC0,0xC3,0xC3,0xC5,0xC5,0xC6,0xC6,0xC9,0xC9,0xCA,0xCA,0xCC,0xCC,0xCF,0xCF,
	0xD1,0xD1,0xD2,0xD2,0xD4,0xD4,0xD7,0xD7,0xD8,0xD8,0xDB,0xDB,0xDD,0xDD,0xDE,0xDE,
	0xE1,0xE1,0xE2,0xE2,0xE4,0xE4,0xE7,0xE7,0xE8,0xE8,0xEB,0xEB,0xED,0xED,0xEE,0xEE,
	0xF0,0xF0,0xF3,0xF3,0xF5,0xF5,0xF6,0xF6,0xF9,0xF9,0xFA,0xFA,0xFC,0xFC,0xFF,0xFF,
};

static const char *_charset[256] = {
	"Ã","Å","Æ","Œ","ŷ","Ý","Õ","Ø","Þ","Ŋ","Ŕ","Ć","Ś","Ź","Ŧ","ð",
	"ã","å","æ","œ","ŵ","ý","õ","ø","þ","ŋ","ŕ","ć","ś","ź","ŧ","",
	" ","!","\"","#","¤","%","&","'","(",")","*","+",",","-",".","/",
	"0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
	"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
	"P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","―","_",
	"‖","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
	"p","q","r","s","t","u","v","w","x","y","z","{","|","}","¯","",
	"á","à","é","è","í","ì","ó","ò","ú","ù","Ñ","Ç","Ş","β","¡","Ĳ",
	"â","ä","ê","ë","î","ï","ô","ö","û","ü","ñ","ç","ş","ǧ","ı","ĳ",
	"ª","α","©","‰","Ǧ","ě","ň","ő","π","₠","£","$","←","↑","→","↓",
	"º","¹","²","³","±","İ","ń","ű","µ","¿","÷","°","¼","½","¾","§",
	"Á","À","É","È","Í","Ì","Ó","Ò","Ú","Ù","Ř","Č","Š","Ž","Ð","Ŀ",
	"Â","Ä","Ê","Ë","Î","Ï","Ô","Ö","Û","Ü","ř","č","š","ž","đ","ŀ",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
};

typedef struct {
	int number;
	const char *programme_type;
	const char *short_type;
	int music;
} _programme_type_t;

/*static const _programme_type_t _programme_types[16] = {
	{  0, "No programme type or undefined", "UNDEF",   -1 },
	{  1, "News",                           "NEWS",     0 },
	{  2, "Current affairs",                "AFFAIRS",  0 },
	{  3, "Information",                    "INFO",     0 },
	{  4, "Sport",                          "SPORT",    0 },
	{  5, "Education",                      "EDUCATE",  0 },
	{  6, "Drama",                          "DRAMA",    0 },
	{  7, "Culture",                        "CULTURES", 0 },
	{  8, "Science",                        "SCIENCE",  0 },
	{  9, "Varied",                         "VARIED",   0 },
	{ 10, "Pop music",                      "POP M",    1 },
	{ 11, "Rock music",                     "ROCK M",   1 },
	{ 12, "M.O.R. music",                   "M.O.R. M", 1 },
	{ 13, "Light classical",                "LIGHT M",  1 },
	{ 14, "Serious classical",              "CLASSICS", 1 },
	{ 15, "Other music",                    "OTHER M",  1 },
};*/

typedef struct {
	int shift;
	uint16_t mask;
} _comp_range_t;

static const _comp_range_t _ranges[8] = {
	{ 7, 0x7F00 },
	{ 6, 0x7E00 },
	{ 5, 0x7C00 },
	{ 4, 0x7800 },
	{ 3, 0x7000 },
	{ 2, 0x6000 },
	{ 1, 0x4000 },
	{ 0, 0x0000 },
};

/* Abbreviated BCH(14,6) checkbits for ZI frame scale factors */
static const uint8_t _zi_bch[64] = {
	0x00,0xD1,0x73,0xA2,0xE6,0x37,0x95,0x44,
	0x1D,0xCC,0x6E,0xBF,0xFB,0x2A,0x88,0x59,
	0x3A,0xEB,0x49,0x98,0xDC,0x0D,0xAF,0x7E,
	0x27,0xF6,0x54,0x85,0xC1,0x10,0xB2,0x63,
	0x74,0xA5,0x07,0xD6,0x92,0x43,0xE1,0x30,
	0x69,0xB8,0x1A,0xCB,0x8F,0x5E,0xFC,0x2D,
	0x4E,0x9F,0x3D,0xEC,0xA8,0x79,0xDB,0x0A,
	0x53,0x82,0x20,0xF1,0xB5,0x64,0xC6,0x17,
};

static uint32_t _utf8next(const char *str, const char **next)
{
	const uint8_t *c;
	uint32_t u, m;
	uint8_t b;
	
	/* Read and return a utf-8 character from str.
	 * If next is not NULL, it is pointed to the next
	 * character following this.
	 * 
	 * If an invalid code is detected, the function
	 * returns U+FFFD, � REPLACEMENT CHARACTER.
	*/
	
	c = (const uint8_t *) str;
	if(next) *next = str + 1;
	
	/* Shortcut for single byte codes */
	if(*c < 0x80) return(*c);
	
	/* Find the code length, initial bits and the first valid code */
	if((*c & 0xE0) == 0xC0) { u = *c & 0x1F; b = 1; m = 0x00080; }
	else if((*c & 0xF0) == 0xE0) { u = *c & 0x0F; b = 2; m = 0x00800; }
	else if((*c & 0xF8) == 0xF0) { u = *c & 0x07; b = 3; m = 0x10000; }
	else return(0xFFFD);
	
	while(b--)
	{
		/* All bytes after the first must begin 0x10xxxxxx */
		if((*(++c) & 0xC0) != 0x80) return(0xFFFD);
		
		/* Add the 6 new bits to the code */
		u = (u << 6) | (*c & 0x3F);
		
		/* Advance next pointer */
		if(next) (*next)++;
	}
	
	/* Reject overlong encoded characters */
	if(u < m) return(0xFFFD);
	
	return(u);
}

static void _mkprbs(uint8_t *b, int type)
{
	uint16_t r = 0xBD;
	int x, bit;
	
	for(x = 12; x < 320; x++)
	{
		bit = (type ? r ^ (r >> 3) : r) & 1;
		b[x >> 3] ^= bit << (7 - (x & 7));
		
		bit = (r ^ (r >> 4)) & 1;
		r = (r >> 1) | (bit << 8);
	}
}

static void _bch_encode_63_44(uint8_t *b)
{
	uint32_t code = 0;
	int i, bit;
	
	for(i = 0; i < 44; i++)
	{
		bit = (b[i >> 3] >> (7 - (i & 7))) & 1;
		bit = (bit ^ (code >> 18)) & 1;
		
		code <<= 1;
		
		if(bit) code ^= 0x8751;
	}
	
	bits_write_uint(b, 44, code, 19);
}

static void _77block(uint8_t *b, int16_t l1, int16_t r1, int16_t l2, int16_t r2, int zi1, int zi2)
{
	bits_write_int(b,  0, l1 >> 3, 11);
	bits_write_int(b, 11, r1 >> 3, 11);
	bits_write_int(b, 22, l2 >> 3, 11);
	bits_write_int(b, 33, r2 >> 3, 11);
	_bch_encode_63_44(b);
	
	bits_write_uint(b, 63, zi1, 1);
	bits_write_uint(b, 64, zi2, 1);
	
	bits_write_int(b, 65, l1, 3);
	bits_write_int(b, 68, r1, 3);
	bits_write_int(b, 71, l2, 3);
	bits_write_int(b, 74, r2, 3);
}

static void _ziframe(uint8_t *b, uint8_t sc_l, uint8_t sc_r, uint32_t pi)
{
	uint16_t c;
	
	c = ((sc_l & 7) << 3) | (sc_r & 7);
	c = (c << 8) | _zi_bch[c];
	
	bits_write_int(b,  0,  c, 14);
	bits_write_int(b, 14,  c, 14);
	bits_write_int(b, 28,  c, 14);
	bits_write_int(b, 42, pi, 22);
}

extern void dsr_encode(dsr_t *s, uint8_t *block, const int16_t *audio)
{
	int i, j, x;
	uint8_t a[40], b[40];
	uint8_t c[8][10];
	uint8_t zi[16][8];
	int16_t as, *ac;
	const int16_t *ap;
	const _comp_range_t *scale[32];
	int blockno;
	
	/* Calculate the audio block number */
	blockno = s->frame >> 6;
	
	/* Calculate the scale for each channel */
	for(ap = audio, i = 0; i < 32; i++)
	{
		/* Default to the minimum scale */
		scale[i] = _ranges;
		
		for(x = 0; x < 64; x++, ap++)
		{
			as = (*ap < 0 ? ~*ap : *ap);
			while(as & scale[i]->mask)
			{
				scale[i]++;
			}
		}
	}
	
	/* Encode the ZI frames */
	for(i = 0; i < 16; i++)
	{
		_ziframe(zi[i], scale[i * 2 + 0]->shift, scale[i * 2 + 1]->shift, 0);
	}
	
	/* Load the new audio data into the delay buffer (+4ms) */
	ac = &s->delay[(((blockno + 2) & 3) * 0x800) & 0x1FFF];
	for(x = 0; x < 64; x++)
	{
		for(i = 0; i < 32; i++, ac++)
		{
			*ac = audio[i * 64 + x] << scale[i]->shift;
			*ac >>= 2;
		}
	}
	
	/* Move the audio pointer back to previously written samples (-4ms) */
	ac = &s->delay[((blockno & 3) * 0x800) & 0x1FFF];
	
	/* Generate the 64 main frame pairs for this audio block */
	for(i = 0; i < 64; i++)
	{
		/* Clear the frames */
		memset(a, 0, 40);
		memset(b, 0, 40);
		
		/* Sync word */
		bits_write_uint(a, 0,  0x712, 11);
		bits_write_uint(b, 0, ~0x712, 11);
		
		/* Special service bit */
		j = s->frame + 16; /* SA bits are offset by 16 bits from the audio blocks */
		bits_write_uint(a, 11, s->sa[(j >> 6) & 127][(j >> 3) & 7] >> (7 - (j & 7)), 1);
		bits_write_uint(b, 11, 0, 1);
		
		/* Generate the 77-bit blocks */
		for(j = 0; j < 8; j++, ac += 4)
		{
			_77block(c[j],
				ac[0], ac[1], ac[2], ac[3],
				zi[j * 2 + 0][i >> 3] >> (7 - (i & 7)),
				zi[j * 2 + 1][i >> 3] >> (7 - (i & 7))
			);
			c[j][9] >>= 3;
		}
		
		/* Insert the 77-bit blocks into the frames, 2x interleaved */
		for(x = j = 0; j < 10; j++)
		{
			int l = (j == 9 ? 10 : 16);
			bits_write_uint(a,  12 + x, (_ileave[c[0][j]] << 1) | (_ileave[c[1][j]] << 0), l);
			bits_write_uint(a, 166 + x, (_ileave[c[2][j]] << 1) | (_ileave[c[3][j]] << 0), l);
			bits_write_uint(b,  12 + x, (_ileave[c[4][j]] << 1) | (_ileave[c[5][j]] << 0), l);
			bits_write_uint(b, 166 + x, (_ileave[c[6][j]] << 1) | (_ileave[c[7][j]] << 0), l);
			x += l;
		}
		
		/* Apply spectrum shaping PRBS */
		_mkprbs(a, 0);
		_mkprbs(b, 1);
		
		/* Interleave the two new frames into the output */
		for(j = 0; j < 40; j++, block += 2)
		{
			block[0] = (_ileave[a[j]] >> 7) | (_ileave[b[j]] >> 8);
			block[1] = (_ileave[a[j]] << 1) | (_ileave[b[j]] << 0);
		}
		
		s->frame++;
	}
}

void dsr_encode_ps(uint8_t *dst, const char *src)
{
	uint32_t c;
	int i, j;
	
	for(i = 0; i < 8 && *src; i++)
	{
		c = _utf8next(src, &src);
		
		/* Lookup DSR character set */
		for(j = 0; j < 256; j++)
		{
			if(c == _utf8next(_charset[j], NULL)) break;
		}
		
		/* Write character, or ' ' if not recognised */
		dst[i] = (j == 256 ? ' ' : j);
	}
	
	for(; i < 8; i++)
	{
		dst[i] = ' ';
	}
}

void dsr_decode_ps(char *dst, const uint8_t *src)
{
	int i;
	
	for(*dst = '\0', i = 0; i < 8; i++, src++)
	{
		strcat(dst, _charset[*src][0] ? _charset[*src] : "?");
	}
}

void dsr_update_sa(dsr_t *s)
{
	dsr_channel_t *c;
	int i, b;
	
	/* Generate the SAÜ/PA (programme information) frames (test data) */
	for(i = 0; i < 56; i++)
	{
		c = &s->channels[(i & 7) * 4];
		
		bits_write_uint(s->sa[i], 0, i & 7 ? 0x5FF : 0x5CF, 16);
		s->sa[i][2] = _par[(c[0].type << 4) | (c[0].music << 3) | (c[0].mode << 1)];
		s->sa[i][3] = _par[(c[1].type << 4) | (c[1].music << 3) | (c[1].mode << 1)];
		s->sa[i][4] = _par[(c[2].type << 4) | (c[2].music << 3) | (c[2].mode << 1)];
		s->sa[i][5] = _par[(c[3].type << 4) | (c[3].music << 3) | (c[3].mode << 1)];
		s->sa[i][6] = 0x00; /* DI */
		s->sa[i][7] = 0x00; /* DII */
	}
	
	/* Generate the SAÜ/LB (zero byte) frames */
	for(; i < 64; i++)
	{
		bits_write_uint(s->sa[i], 0, i & 7 ? 0x5FF : 0x5CF, 16);
		s->sa[i][2] = 0x00;
		s->sa[i][3] = 0x00;
		s->sa[i][4] = 0x00;
		s->sa[i][5] = 0x00;
		s->sa[i][6] = 0x00;
		s->sa[i][7] = 0x00;
	}
	
	/* Generate the SAÜ/SK (programme source) frames (test data) */
	for(; i < 128; i++)
	{
		c = &s->channels[(i & 7) * 4];
		b = (i - 64) >> 3;
		
		bits_write_uint(s->sa[i], 0, i & 7 ? 0x5FF : 0x5CF, 16);
		s->sa[i][2] = c[0].name[b];
		s->sa[i][3] = c[1].name[b];
		s->sa[i][4] = c[2].name[b];
		s->sa[i][5] = c[3].name[b];
		s->sa[i][6] = 0x00; /* EI */
		s->sa[i][7] = 0x00; /* EII */
	}
}

void dsr_init(dsr_t *s)
{
	int i;
	
	memset(s, 0, sizeof(dsr_t));
	
	/* Initial channel setup (all disabled) */
	for(i = 0; i < 32; i++)
	{
		s->channels[i].music = 1;
	}
	
	dsr_update_sa(s);
}

