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

#ifndef _CONF_H
#define _CONF_H

typedef char* conf_t;

/* These functions load and parse an ini-style configuration file:
 * 
 * ; Comment
 * [section]
 * key = value
 * 
 * A key must have a value, even if it is empty:
 * 
 * key = ; Valid
 * key   ; Invalid
 * 
 * Comments may appear at the end of any line:
 * 
 * ; Comment
 * [section] ; Hello
 * key1 = value ; Yep, this is valid
 * key2 = "hey ;-)" ; Use quotes if the value contains a semi-colon
 * 
 * Leading and trailing spaces are trimmed from keys and values. Use quotes
 * if a value must begin or end with spaces:
 * 
 * key = "  the final frontier  "
 * 
 * C-style escapes can be used inside quoted strings.
 * \" \\ \t \n and \r are supported.
 * 
 * key = "this is a\nmulti line string"
 * 
 * Keys that appear before a section decleration are part of the NULL section.
 * 
 * Sections may appear multiple times. Use index 0+ to read a key from a
 * specific section.
 * 
 * Use index -1 to read a key from any iteration of any occurrence of a
 * section.
 * 
 * If the same key appears multiple times within a section, only the first
 * matching instance is used.
*/

extern conf_t conf_loadfile(const char *filename);
extern int conf_section_exists(const conf_t conf, const char *section, int index);
extern int conf_key_exists(const conf_t conf, const char *section, int index, const char *key);
extern const char *conf_str(const conf_t conf, const char *section, int index, const char *key, const char *defaultval);
extern long int conf_int(const conf_t conf, const char *section, int index, const char *key, long int defaultval);
extern double conf_double(const conf_t conf, const char *section, int index, const char *key, double defaultval);
extern int conf_bool(const conf_t conf, const char *section, int index, const char *key, int defaultval);

#endif

