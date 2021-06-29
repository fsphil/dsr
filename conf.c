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
#include <stdlib.h>
#include <string.h>
#include "conf.h"

static void _conf_process(char **d, char **s, const char *accept, const char *reject)
{
	/* Process the characters at *s until either a \0 terminator, a
	 * character not in *accept (if not NULL), or a character in
	 * *reject (if not NULL) is found.
	 * 
	 * On return the *s pointer points at the first character not to
	 * satisfy the above conditions. If *d is not NULL, the matching
	 * characters are copied there. On return the *d pointer points to
	 * the next output byte.
	*/
	
	while(**s != '\0')
	{
		if(accept && strchr(accept, **s) == NULL) break;
		if(reject && strchr(reject, **s) != NULL) break;
		if(d) *(*d)++ = **s;
		
		(*s)++;
	}
}

size_t _conf_parse(const char *name, char *s)
{
	int line;
	char *d, *t, *os;
	
	/* Parses and overwrites string at s. The result is a series of strings,
	 * with the end flagged with a zero length string. Section decleration
	 * strings retain the initial '['.
	 * 
	 * key\0 value\0 [section\0 key\0 value\0 ... \0
	*/
	
	for(line = 1, os = d = s; *s; line++)
	{
		/* Skip any space at the start of the line */
		_conf_process(NULL, &s, " \t", NULL);
		
		switch(*s)
		{
		case '\0': /* EOF */
		case '\n': /* Newline */
		case '\r': /* Newline */
		case ';': /* Comment */
			/* Skip this line */
			break;
		
		case '=': /* Keyless value */
			fprintf(stderr, "%s: Warning: Keyless value on line %d\n", name, line);
			break;
		
		case '[': /* Section identifier */
			_conf_process(&d, &s, NULL, "]\n\r");
			
			if(*s != ']') fprintf(stderr, "%s: Warning: Missing ] on line %d\n", name, line);
			if(*s != '\0') s++;
			
			/* Test for unexpected non-comment text after the section identifier */
			_conf_process(NULL, &s, " \t", "\n\r");
			
			if(*s != '\0' && *s != '\n' && *s != '\r' && *s != ';')
			{
				fprintf(stderr, "%s: Warning: Unexpected text after section identifier on line %d\n", name, line);
			}
			
			/* Terminate the identifier string */
			*(d++) = '\0';
			
			break;
		
		default: /* Key identifier */
			t = d;
			_conf_process(&d, &s, NULL, "=\n\r");
			
			if(*s != '=')
			{
				fprintf(stderr, "%s: Warning: Valueless key on line %d\n", name, line);
				
				/* Rewind output pointer to drop this key */
				d = t;
				break;
			}
			
			/* Remove any trailing space from the key */
			while(*(d - 1) == ' ' || *(d - 1) == '\t')
			{
				d--;
			}
			
			/* Terminate the key string */
			*(d++) = '\0';
			
			/* Skip the equals and any space at the start of the value */
			s++;
			_conf_process(NULL, &s, " \t", NULL);
			
			/* Test if this is a quoted string */
			if(*s == '"')
			{
				do
				{
					if(*(s++) == '\\')
					{
						/* Found an escape sequence */
						switch(*s)
						{
						case '"':  *(d++) = '"'; break;
						case '\\': *(d++) = '\\'; break;
						case 't':  *(d++) = '\t'; break;
						case 'n':  *(d++) = '\n'; break;
						case 'r':  *(d++) = '\r'; break;
						default: fprintf(stderr, "%s: Warning: Unrecognised escape sequence \\%c on line %d\n", name, *s, line);
						}
						
						s++;
					}
					
					_conf_process(&d, &s, NULL, "\\\"\n\r");
				}
				while(*s == '\\');
				
				if(*s != '"') fprintf(stderr, "%s: Warning: Missing quotation mark at end of line %d\n", name, line);
				if(*s != '\0') s++;
				
				/* Test for unexpected non-comment text after the quoted value */
				_conf_process(NULL, &s, " \t", "\n\r");
				
				if(*s != '\0' && *s != '\n' && *s != '\r' && *s != ';')
				{
					fprintf(stderr, "%s: Warning: Unexpected text after value on line %d\n", name, line);
				}
			}
			else
			{
				/* Read until a comment or the end of the line */
				_conf_process(&d, &s, NULL, ";\n\r");
				
				/* Remove any trailing space from the value */
				while(*(d - 1) == ' ' || *(d - 1) == '\t')
				{
					d--;
				}
			}
			
			/* Terminate the value string */
			*(d++) = '\0';
		}
		
		/* Skip remainder of this line */
		_conf_process(NULL, &s, NULL, "\n\r");
		if(*s != '\0') s++;
	}
	
	/* End of config termination */
	*(d++) = '\0';
	
	return(d - os);
}

conf_t conf_loadfile(const char *filename)
{
	FILE *f;
	long int l, r;
	char *s, *p;
	
	/* Open the file */
	f = fopen(filename, "rb");
	if(!f)
	{
		perror(filename);
		return(NULL);
	}
	
	/* Find it's total length */
	fseek(f, 0, SEEK_END);
	l = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	/* Allocate enough memory for the file, plus two bytes for conf terminators */
	s = malloc(l + 2);
	if(!s)
	{
		perror(filename);
		fclose(f);
		return(NULL);
	}
	
	/* Load the entire file into memory */
	for(p = s; l; p += r, l -= r)
	{
		r = fread(s, 1, l, f);
		if(r <= 0) break;
	}
	
	fclose(f);
	
	if(l)
	{
		/* fread() returned an error */
		perror(filename);
		free(s);
		return(NULL);
	}
	
	/* Null terminate the file */
	*p = '\0';
	
	/* Run the file through the configuration parser */
	r = _conf_parse(filename, s);
	
	/* Free up unused memory. Not really necessary */
	p = realloc(s, r);
	if(p) s = p;
	
	/* All done */
	return(s);
}

const char *_conf_find(const conf_t conf, const char *section, int index, const char *key)
{
	const char *g, *s;
	int m;
	
	if(key == NULL) return(NULL);
	
	g = NULL;
	s = conf;
	m = section == NULL;
	
	while(*s)
	{
		/* Start of a new section */
		if(*s == '[')
		{
			if(m)
			{
				if(index == 0) return(NULL);
				else if(index > 0) index--;
			}
			
			g = s + 1;
			m = section != NULL && strcasecmp(g, section) == 0;
		}
		else
		{
			const char *k = s;
			
			s += strlen(s) + 1;
			
			if(m && index <= 0 && strcasecmp(k, key) == 0)
			{
				/* Found a match. Return a pointer to the value */
				return(s);
			}
		}
		
		/* Move to next string */
		s += strlen(s) + 1;
	}
	
	/* Nothing found */
	return(NULL);
}

int conf_section_exists(const conf_t conf, const char *section, int index)
{
	const char *s = conf;
	
	/* The default section always exists */
	if(section == NULL) return(-1);
	
	while(*s)
	{
		if(*s == '[')
		{
			/* The start of a new section */
			if(strcasecmp(s + 1, section) == 0)
			{
				/* Found a matching section */
				if(--index < 0) return(-1);
			}
			
			s += strlen(s) + 1;
		}
		else
		{
			/* Skip this key value pair */
			s += strlen(s) + 1;
			s += strlen(s) + 1;
		}
	}
	
	/* The section was not found */
	return(0);
}

int conf_key_exists(const conf_t conf, const char *section, int index, const char *key)
{
	return(_conf_find(conf, section, index, key) == NULL ? 0 : -1);
}

const char *conf_str(const conf_t conf, const char *section, int index, const char *key, const char *defaultval)
{
	const char *v = _conf_find(conf, section, index, key);
	return(v == NULL ? defaultval : v);
}

long int conf_int(const conf_t conf, const char *section, int index, const char *key, long int defaultval)
{
	const char *v = _conf_find(conf, section, index, key);
	return(v == NULL ? defaultval : strtol(v, NULL, 0));
}

double conf_double(const conf_t conf, const char *section, int index, const char *key, double defaultval)
{
	const char *v = _conf_find(conf, section, index, key);
	return(v == NULL ? defaultval : strtod(v, NULL));
}

extern int conf_bool(const conf_t conf, const char *section, int index, const char *key, int defaultval)
{
	const char *v = _conf_find(conf, section, index, key);
	
	/* Return default value if key isn't found */
	
	if(!v) return(defaultval);
	
	/* Return -1 (true) if key contains "true" or "yes" */
	
	if(strcasecmp(v, "true") == 0 ||
	   strcasecmp(v, "yes") == 0) return(-1);
	
	/* Return 0 (false) if key contains "false" or "no" */
	
	if(strcasecmp(v, "false") == 0 ||
	   strcasecmp(v, "no") == 0) return(0);
	
	/* Return 0 (false) if key == 0, otherwise -1 */
	return(strtol(v, NULL, 0) == 0 ? 0 : -1);
}

