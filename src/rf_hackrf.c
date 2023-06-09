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
#include <stdlib.h>
#include <string.h>
#include <libhackrf/hackrf.h>
#include <pthread.h>
#include <unistd.h>
#include "rf.h"

#define BUFFERS 32

typedef struct {
	
	/* Buffers are locked while reading/writing */
	pthread_mutex_t mutex;
	
	/* Pointer to the start of the buffer */
	int8_t *data;
	
	/* Offset to start of data */
	size_t start;
	
	/* Length of data ready */
	size_t length;
	
} buffer_t;

typedef struct {
	
	buffer_t *buffers;
	
	int length;
	int count;
	int in;
	int out;
	
} buffers_t;

typedef struct {
	
	/* HackRF device */
	hackrf_device *d;
	
	/* Buffers */
	buffers_t buffers;
	
} hackrf_t;

static int _buffer_init(buffers_t *buffers, size_t count, size_t length)
{
	int i;
	
	buffers->count = count;
	buffers->length = length;
	buffers->buffers = calloc(count, sizeof(buffer_t));
	
	for(i = 0; i < count; i++)
	{
		pthread_mutex_init(&buffers->buffers[i].mutex, NULL);
		buffers->buffers[i].data = malloc(length);
		buffers->buffers[i].start = 0;
		buffers->buffers[i].length = 0;
	}
	
	/* Lock the initial buffer for the provider */
	buffers->in = 0;
	pthread_mutex_lock(&buffers->buffers[buffers->in].mutex);
	
	/* Lock the last empty buffer for the consumer */
	buffers->out = count - 1;
	pthread_mutex_lock(&buffers->buffers[buffers->out].mutex);

	return(0);
}

static int _buffer_free(buffers_t *buffers)
{
	int i;
	
	for(i = 0; i < buffers->count; i++)
	{
		free(buffers->buffers[i].data);
		pthread_mutex_destroy(&buffers->buffers[i].mutex);
	}
	
	free(buffers->buffers);
	memset(buffers, 0, sizeof(buffers_t));
	
	return(0);
}

static int _buffer_read(buffers_t *buffers, void *dst, size_t length)
{
	buffer_t *buf = &buffers->buffers[buffers->out];
	
	if(buf->length == 0)
	{
		buffer_t *next;
		int i;
		
		/* This buffer is empty, try to move the read lock onto the next one */
		i = (buffers->out + 1) % buffers->count;
		next = &buffers->buffers[i];
		
		if(pthread_mutex_trylock(&next->mutex) != 0)
		{
			/* No luck, the writer must have it */
			fprintf(stderr, "U");
			return(0);
		}
		
		/* Got a lock on the next buffer, clear and release the previous */
		buf->start = 0;
		pthread_mutex_unlock(&buf->mutex);
		
		buf = next;
		buffers->out = i;
	}
	
	if(length > buf->length)
	{
		length = buf->length;
	}
	
	memcpy(dst, buf->data + buf->start, length);
	buf->start += length;
	buf->length -= length;
	
	return(length);
}

static int _buffer_write(buffers_t *buffers, void *src, size_t length)
{
	buffer_t *buf = &buffers->buffers[buffers->in];
	int i;
	
	if(buf->length == buffers->length)
	{
		buffer_t *next;
		
		/* This buffer is full, move the write lock onto the next one */
		i = (buffers->in + 1) % buffers->count;
		next = &buffers->buffers[i];
		
		pthread_mutex_lock(&next->mutex);
		pthread_mutex_unlock(&buf->mutex);
		
		buf = next;
		buffers->in = i;
	}
	
	i = buf->start + buf->length;
	if(length > buffers->length - i)
	{
		length = buffers->length - i;
	}
	
	memcpy(buf->data + i, src, length);
	buf->length += length;

	return(length);
}

static int _tx_callback(hackrf_transfer *transfer)
{
	hackrf_t *rf = transfer->tx_ctx;
	size_t l = transfer->valid_length;
	uint8_t *buf = transfer->buffer;
	int r;
	
	while(l)
	{
		r = _buffer_read(&rf->buffers, buf, l);
		
		if(r == 0)
		{
			/* Buffer underrun, fill with zero */
			memset(buf, 0, l);
			l = 0;
		}
		else
		{
			l -= r;
			buf += r;
		}
	}
	
	return(0);
}

static int _rf_write(void *private, int16_t *iq_data, int samples)
{
	hackrf_t *rf = private;
	int8_t iq8[4096];
	int i, r, b;
	
	samples *= 2;
	
	while(samples > 0)
	{
		b = samples;
		if(b > 4096) b = 4096;
		
		for(i = 0; i < b; i++)
		{
			iq8[i] = *(iq_data++) >> 8;
			samples--;
		}
		
		i = 0;
		while(b)
		{
			r = _buffer_write(&rf->buffers, &iq8[i], b);
			
			b -= r;
			i += r;
		}
	}
	
	return(0);
}

static int _rf_close(void *private)
{
	hackrf_t *rf = private;
	int r;
	
	r = hackrf_stop_tx(rf->d);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_stop_tx() failed: %s (%d)\n", hackrf_error_name(r), r);
		return(-1);
	}
	
	/* Wait until streaming has stopped */
	while(hackrf_is_streaming(rf->d) == HACKRF_TRUE)
	{
		usleep(100);
	}
	
	r = hackrf_close(rf->d);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(r), r);
	}
	
	hackrf_exit();
	
	_buffer_free(&rf->buffers);
	free(rf);
	
	return(0);
}

int rf_hackrf_open(rf_t *s, const char *serial, int sample_rate, uint64_t frequency_hz, unsigned int txvga_gain, unsigned char amp_enable)
{
	hackrf_t *rf;
	int r;
	
	rf = calloc(1, sizeof(hackrf_t));
	if(!rf)
	{
		return(-1);
	}
	
	/* Allocate 0.5 seconds for output buffers */
	_buffer_init(&rf->buffers, BUFFERS, sample_rate * 2 / BUFFERS);
	
	/* Prepare the HackRF for output */
	r = hackrf_init();
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_open_by_serial(serial, &rf->d);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_set_sample_rate_manual(rf->d, sample_rate, 1);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_set_baseband_filter_bandwidth(rf->d, hackrf_compute_baseband_filter_bw(sample_rate));
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_set_freq(rf->d, frequency_hz);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_set_txvga_gain(rf->d, txvga_gain);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_set_txvga_gain() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_set_amp_enable(rf->d, amp_enable);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_set_amp_enable() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	r = hackrf_start_tx(rf->d, _tx_callback, rf);
	if(r != HACKRF_SUCCESS)
	{
		fprintf(stderr, "hackrf_start_tx() failed: %s (%d)\n", hackrf_error_name(r), r);
		free(rf);
		return(-1);
	}
	
	/* Register the callback functions */
	s->private = rf;
	s->write = _rf_write;
	s->close = _rf_close;
	
	return(0);
};

