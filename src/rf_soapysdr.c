/* dsr - Digitale Satelliten Radio (DSR) encoder                         */
/*=======================================================================*/
/* Copyright 2022 Philip Heron <phil@sanslogic.co.uk>                    */
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
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Version.h>
#include "rf.h"

typedef struct {
	
	/* SoapySDR device and stream */
	SoapySDRDevice *d;
	SoapySDRStream *s;
	
} soapysdr_t;

static int _rf_write(void *private, int16_t *iq_data, int samples)
{
	soapysdr_t *rf = private;
	const void *buffs[1];
	int flags = 0;
	int r;
	
	while(samples > 0)
	{
		buffs[0] = iq_data;
		
		r = SoapySDRDevice_writeStream(rf->d, rf->s, buffs, samples, &flags, 0, 100000);
		
		if(r <= 0)
		{
			return(-1);
		}
		
		samples -= r;
		iq_data += r * 2;
	}
	
	return(0);
}

static int _rf_close(void *private)
{
	soapysdr_t *rf = private;
	
	SoapySDRDevice_deactivateStream(rf->d, rf->s, 0, 0);
	SoapySDRDevice_closeStream(rf->d, rf->s);
	
	SoapySDRDevice_unmake(rf->d);
	
	return(0);
}

int rf_soapysdr_open(rf_t *s, const char *device, unsigned int sample_rate, unsigned int frequency_hz, unsigned int gain, const char *antenna)
{
	soapysdr_t *rf;
	SoapySDRKwargs *results;
	size_t length;
	char *sn;
	double fullscale;
	
	rf = calloc(1, sizeof(soapysdr_t));
	if(!rf)
	{
		return(-1);
	}
	
	/* Display a list of devices */
	results = SoapySDRDevice_enumerate(NULL, &length);
	
	if(length <= 0)
	{
		fprintf(stderr, "No SoapySDR devices found.\n");
		free(rf);
		return(-1);
	}
	
	/*for(i = 0; i < length; i++)
	{
		fprintf(stderr, "Found device #%ld: ", i);
		
		for(j = 0; j < results[i].size; j++)
		{
			fprintf(stderr, "%s=%s, ", results[i].keys[j], results[i].vals[j]);
		}
		
		fprintf(stderr, "\n");
	}*/
	
	SoapySDRKwargsList_clear(results, length);
	
	/* Prepare the device for output */
	rf->d = SoapySDRDevice_makeStrArgs(device);
	
	if(rf->d == NULL)
	{
		fprintf(stderr, "SoapySDRDevice_make() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	if(SoapySDRDevice_setSampleRate(rf->d, SOAPY_SDR_TX, 0, sample_rate) != 0)
	{
		fprintf(stderr, "SoapySDRDevice_setSampleRate() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	if(SoapySDRDevice_setBandwidth(rf->d, SOAPY_SDR_TX, 0, 20e6) != 0)
	{
		fprintf(stderr, "SoapySDRDevice_setBandwidth() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	if(SoapySDRDevice_setFrequency(rf->d, SOAPY_SDR_TX, 0, frequency_hz, NULL) != 0)
	{
		fprintf(stderr, "SoapySDRDevice_setFrequency() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	if(SoapySDRDevice_setGain(rf->d, SOAPY_SDR_TX, 0, gain) != 0)
	{
		fprintf(stderr, "SoapySDRDevice_setGain() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	if(antenna && SoapySDRDevice_setAntenna(rf->d, SOAPY_SDR_TX, 0, antenna) != 0)
	{
		fprintf(stderr, "SoapySDRDevice_setAntenna() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	/* Query the native stream format, see if we need to scale the output */
	sn = SoapySDRDevice_getNativeStreamFormat(rf->d, SOAPY_SDR_TX, 0, &fullscale);
	if(sn && strcmp(sn, "CS16") == 0)
	{
		s->scale = fullscale / INT16_MAX;
		if(s->scale > 1.0) s->scale = 1.0;
	}
	
#if defined(SOAPY_SDR_API_VERSION) && (SOAPY_SDR_API_VERSION >= 0x00080000)
	rf->s = SoapySDRDevice_setupStream(rf->d, SOAPY_SDR_TX, "CS16", NULL, 0, NULL);
	if(rf->s == NULL)
#else
	if(SoapySDRDevice_setupStream(rf->d, &rf->s, SOAPY_SDR_TX, SOAPY_SDR_CS16, NULL, 0, NULL) != 0)
#endif
	{
		fprintf(stderr, "SoapySDRDevice_setupStream() failed: %s\n", SoapySDRDevice_lastError());
		free(rf);
		return(-1);
	}
	
	SoapySDRDevice_activateStream(rf->d, rf->s, 0, 0, 0);
	
	/* Register the callback functions */
	s->private = rf;
	s->write = _rf_write;
	s->close = _rf_close;
	
	return(0);
};

