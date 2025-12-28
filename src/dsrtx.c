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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include "dsr.h"
#include "conf.h"
#include "src.h"
#include "rf.h"

typedef struct {
	
	/* DSR bitstream encoder */
	dsr_t dsr;
	
	/* RF output */
	rf_qpsk_t qpsk;
	rf_t rf;
	
	const char *output_type;
	const char *output;
	int data_type;
	uint64_t frequency;
	unsigned int sample_rate;
	int gain;
	int amp;
	const char *antenna;
	int live;
	
	/* Verbose flag */
	int verbose;
	
} dsrtx_t;

volatile int _abort = 0;

static void _sigint_callback_handler(int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	
	if(_abort > 0)
	{
		exit(-1);
	}
	
	_abort = 1;
}

static void print_usage(void)
{
	printf(
		"\n"
		"Usage: dsrtx [options]\n"
		"\n"
		"  -c, --config <file>      Load configuration from file.\n"
		"  -v, --verbose            Enable verbose output.\n"
		"\n"
	);
}

static void *_open_src(conf_t conf, int i)
{
	src_t *src;
	const char *v;
	int r;
	
	src = calloc(sizeof(src_t), 1);
	if(!src)
	{
		perror("calloc");
		return(NULL);
	}
	
	v = conf_str(conf, "channel", i, "type", "rawaudio");
	
	if(strcasecmp(v, "rawaudio") == 0)
	{
		v = conf_str(conf, "channel", i, "input", NULL);
		if(!v)
		{
			fprintf(stderr, "Warning: Missing input filename\n");
			free(src);
			return(NULL);
		}
		
		r = src_rawaudio_open(
			src,
			v,
			conf_bool(conf, "channel", i, "exec", 0),
			conf_bool(conf, "channel", i, "stereo", 1),
			conf_bool(conf, "channel", i, "repeat", 0)
		);
		if(r != 0)
		{
			fprintf(stderr, "Warning: Failed to open '%s'\n", v);
			free(src);
			return(NULL);
		}
	}
	else if(strcasecmp(v, "tone") == 0)
	{
		r = src_tone_open(
			src,
			conf_double(conf, "channel", i, "frequency", 0),
			conf_double(conf, "channel", i, "level", 0)
		);
		if(r != 0)
		{
			fprintf(stderr, "Warning: Failed to open tone source\n");
			free(src);
			return(NULL);
		}
	}
#ifdef HAVE_FFMPEG
	else if(strcasecmp(v, "ffmpeg") == 0)
	{
		v = conf_str(conf, "channel", i, "input", NULL);
		if(!v)
		{
			fprintf(stderr, "Warning: Missing input filename/URL\n");
			free(src);
			return(NULL);
		}
		
		r = src_ffmpeg_open(src, v);
		if(r != 0)
		{
			fprintf(stderr, "Warning: Failed to open '%s'\n", v);
			free(src);
			return(NULL);
		}

	}
#endif
	else
	{
		fprintf(stderr, "Warning: Unrecognised input type '%s'\n", v);
		free(src);
		return(NULL);
	}
	
	return(src);
}

const int _load_config(dsrtx_t *s, const char *filename)
{
	conf_t conf;
	const char *v;
	int i;
	int c;
	
	/* Load configuration */
	conf = conf_loadfile(filename);
	if(!conf) return(-1);
	
	/* Load configuration for output */
	s->output_type = strdup(conf_str(conf, "output", -1, "type", "hackrf"));
	s->output = strdup(conf_str(conf, "output", -1, "output", ""));
	
	s->data_type = -1;
	v = conf_str(conf, "output", -1, "data_type", "");
	if(strcmp(v, "uint8") == 0)       s->data_type = RF_UINT8;
	else if(strcmp(v, "int8") == 0)   s->data_type = RF_INT8;
	else if(strcmp(v, "uint16") == 0) s->data_type = RF_UINT16;
	else if(strcmp(v, "int16") == 0)  s->data_type = RF_INT16;
	else if(strcmp(v, "int32") == 0)  s->data_type = RF_INT32;
	else if(strcmp(v, "float") == 0)  s->data_type = RF_FLOAT;
	else if(strcmp(v, "") == 0)       s->data_type = -1;
	else
	{
		fprintf(stderr, "Error: Invalid data type '%s'.\n", v);
		free(conf);
		return(-1);
	}
	
	s->sample_rate = conf_int(conf, "output", -1, "sample_rate", DSR_SYMBOL_RATE * 2);
	s->frequency = conf_double(conf, "output", -1, "frequency", 0),
	s->gain = conf_int(conf, "output", -1, "gain", 0);
	s->amp = conf_int(conf, "output", -1, "amp", 0);
	s->antenna = conf_str(conf, "output", -1, "antenna", NULL);
	s->live = conf_bool(conf, "output", -1, "live", 0);
	
	/* Load configuration for each channel */
	for(i = 0; conf_section_exists(conf, "channel", i); i++)
	{
		c = conf_int(conf, "channel", i, "channel", 0);
		if(c < 1 || c > 16)
		{
			/* Knowing the conf file line number would be handy here */
			fprintf(stderr, "Warning: Invalid or missing channel number. Skipping\n");
			continue;
		}
		c = (c - 1) * 2;
		
		v = conf_str(conf, "channel", i, "mode", "s");
		
		if(strcasecmp(v, "s") == 0)
		{
			/* Stereo channel. Test if both L/R channels are free */
			if(s->dsr.channels[c + 0].mode != 0 ||
			   s->dsr.channels[c + 1].mode != 0)
			{
				fprintf(stderr, "Warning: Channel %02d/S is already allocated. Skipping\n", (c >> 1) + 1);
				continue;
			}
			
			/* Set the L channel parameters */
			dsr_encode_ps(s->dsr.channels[c].name, conf_str(conf, "channel", i, "name", ""));
			s->dsr.channels[c].type = conf_int(conf, "channel", i, "program_type", 0);
			s->dsr.channels[c].music = conf_bool(conf, "channel", i, "music", 0) ? 1 : 0;
			s->dsr.channels[c].mode = 1;
			
			/* Set the R channel parameters */
			dsr_encode_ps(s->dsr.channels[c + 1].name, conf_str(conf, "channel", i, "name", ""));
			s->dsr.channels[c + 1].type = conf_int(conf, "channel", i, "secondary_type", conf_int(conf, "channel", i, "program_type", 0));
			s->dsr.channels[c + 1].music = 0;
			s->dsr.channels[c + 1].mode = 2;
		}
		else if(strcasecmp(v, "a") == 0 ||
		        strcasecmp(v, "b") == 0)
		{
			/* Mono A/B channel */
			if(*v == 'b' || *v == 'B') c++;
			
			/* Test if this channel is free */
			if(s->dsr.channels[c].mode != 0)
			{
				fprintf(stderr, "Warning: Channel %02d/%c is already allocated. Skipping\n", (c >> 1) + 1, c & 1 ? 'B' : 'A');
				continue;
			}
			
			/* Set the channel parameters */
			dsr_encode_ps(s->dsr.channels[c].name, conf_str(conf, "channel", i, "name", ""));
			s->dsr.channels[c].type = conf_int(conf, "channel", i, "program_type", 0);
			s->dsr.channels[c].music = conf_bool(conf, "channel", i, "music", 0) ? 1 : 0;
			s->dsr.channels[c].mode = 1;
		}
		else
		{
			fprintf(stderr, "Warning: Unrecognised channel mode '%s'. Skipping\n", v);
			continue;
		}
		
		/* Open the audio source */
		s->dsr.channels[c].arg = _open_src(conf, i);
	}
	
	s->verbose = conf_bool(conf, NULL, -1, "verbose", s->verbose);
	
	free(conf);
	
	return(0);
}

int main(int argc, char *argv[])
{
	dsrtx_t s;
	const char *conffile = NULL;
	int c, option_index;
	const struct option long_options[] = {
		{ "version", no_argument,       0, 'v' },
		{ "config",  required_argument, 0, 'c' },
		{ "verbose", no_argument,       0, 'V' },
		{ 0, 0, 0, 0 }
	};
	uint8_t block[5120];
	int16_t o2[40960 * 2 * 5];
	int l;
	int16_t audio[64 * 32];
	
#ifdef HAVE_FFMPEG
	src_ffmpeg_init();
#endif
	
	memset(&s, 0, sizeof(dsrtx_t));
	dsr_init(&s.dsr);
	
	opterr = 0;
	while((c = getopt_long(argc, argv, "vc:V", long_options, &option_index)) != -1)
	{
		switch(c)
		{
		case 'v': /* -v, --version */
			fprintf(stderr, "dsrtx v2\n");
			return(0);
		
		case 'c': /* -c, --config <filename> */
			conffile = optarg;
			break;
		
		case 'V': /* -V, --verbose */
			s.verbose = 1;
			break;
		
		case '?':
			print_usage();
			return(0);
		}
	}
	
	if(!conffile)
	{
		fprintf(stderr, "No configuration file specified\n");
		return(-1);
	}
	
	if(_load_config(&s, conffile) == -1)
	{
		/* Configuration error */
		fprintf(stderr, "Failed to load configuration\n");
		return(-1);
	}
	
	if((s.sample_rate % DSR_SYMBOL_RATE) != 0)
	{
		fprintf(stderr, "Sample rate %d is not a multiple of %d.\n", s.sample_rate, DSR_SYMBOL_RATE);
		return(-1);
	}
	
	/* Rebuild SA data */
	dsr_update_sa(&s.dsr);
	
	/* Dump channel configuration */
	if(s.verbose)
	{
		fprintf(stderr, "Active channels:\n");
		
		for(c = 0; c < 32; c++)
		{
			char name[8 * 4 + 1];
			char mode;
			
			if(s.dsr.channels[c & 30].mode == 1 &&
			   s.dsr.channels[(c & 30) + 1].mode == 2)
			{
				mode = c & 1 ? 'R' : 'L';
			}
			else if(s.dsr.channels[c].mode == 1)
			{
				mode = c & 1 ? 'B' : 'A';
			}
			else continue;
			
			dsr_decode_ps(name, s.dsr.channels[c].name);
			fprintf(stderr, "%02d/%c: \"%s\" (Type: %d, Music: %d)\n",
				(c >> 1) + 1, mode, name,
				s.dsr.channels[c].type,
				s.dsr.channels[c].music);
		}
	}
	
	/* Catch all the signals */
	signal(SIGINT, &_sigint_callback_handler);
	signal(SIGILL, &_sigint_callback_handler);
	signal(SIGFPE, &_sigint_callback_handler);
	signal(SIGSEGV, &_sigint_callback_handler);
	signal(SIGTERM, &_sigint_callback_handler);
	signal(SIGABRT, &_sigint_callback_handler);
	
	/* Start the radio */
	if(strcmp(s.output_type, "file") == 0)
	{
		if(rf_file_open(&s.rf, s.output, s.data_type, s.live) != 0)
		{
			return(-1);
		}
	}
#ifdef HAVE_HACKRF
	else if(strcmp(s.output_type, "hackrf") == 0)
	{
		if(rf_hackrf_open(&s.rf, s.output, s.sample_rate, s.frequency, s.gain, s.amp) != 0)
		{
			return(-1);
		}
	}
#endif
#ifdef HAVE_SOAPYSDR
	else if(strcmp(s.output_type, "soapysdr") == 0)
	{
		if(rf_soapysdr_open(&s.rf, s.output, s.sample_rate, s.frequency, s.gain, s.antenna) != 0)
		{
			return(-1);
		}
	}
#endif
	else
	{
		fprintf(stderr, "Unrecognised output type: %s\n", s.output_type);
		return(-1);
	}
	
	/* Initalise the modem */
	rf_qpsk_init(&s.qpsk, s.sample_rate / DSR_SYMBOL_RATE, 0.8 * rf_scale(&s.rf));
	
	while(!_abort)
	{
		/* Update the audio block */
		memset(audio, 0, 64 * 32 * sizeof(int16_t));
		
		for(l = 0; l < 32; l++)
		{
			if(s.dsr.channels[l & 30].mode == 1 &&
			   s.dsr.channels[(l & 30) + 1].mode == 2)
			{
				src_read_stereo(s.dsr.channels[l].arg, &audio[l * 64], 1, &audio[(l + 1) * 64], 1, 64);
				l++;
			}
			else if(s.dsr.channels[l].mode == 1)
			{
				src_read_mono(s.dsr.channels[l].arg, &audio[l * 64], 1, 64);
			}
		}
		
		/* Encode the next audio block (2ms) */
		dsr_encode(&s.dsr, block, audio);
		
		/* New version */
		l = rf_qpsk_modulate(&s.qpsk, o2, block, 40960);
		rf_write(&s.rf, o2, l);
	}
	
	rf_close(&s.rf);
	
	/* Close each source */
	for(c = 0; c < 32; c++)
	{
		if(s.dsr.channels[c].arg)
		{
			src_close(s.dsr.channels[c].arg);
			free(s.dsr.channels[c].arg);
		}
	}
	
#ifdef HAVE_FFMPEG
	src_ffmpeg_deinit();
#endif
	
	return(0);
}

