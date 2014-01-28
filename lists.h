/* $Id: lists.h,v 1.2 2005/08/19 07:31:06 chris Exp $ */

/*
  Squirm - A Redirector for Squid

  Maintained by Chris Foote, chris@foote.com.au
  Copyright (C) 1998-2005 Chris Foote
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
  Please see the file GPL in this directory for full copyright
  information.
*/


#ifndef LISTS_H

#define LISTS_H
#include<sys/types.h>
#include "regex.h"
#include "ip.h"

#define NORMAL   1
#define EXTENDED 2
#define ABORT    3
#define ABORT_ON_MATCH 4
#define ABORT_NORMAL 5
#define ABORT_EXTENDED 6

#define ACCEL_NORMAL 1
#define ACCEL_START  2
#define ACCEL_END    3


/* the two chief lists */
struct subnet_block *subnet_head;
struct pattern_file *pattern_head;



struct IP_item {
  struct cidr subnet;
  struct IP_item *next;
};


struct REGEX_pattern {
  char *pattern;
  char *replacement;
  int case_sensitive;
  int type;
  int has_accel;
  int accel_type;
  char *accel;
  regex_t cpattern;
};


struct pattern_item {
  struct REGEX_pattern patterns;
  struct pattern_item *next;
};


struct pattern_file {
  struct pattern_item *head;
  char   *filename;
  struct pattern_file *next;
};

struct pattern_block {
  struct pattern_file *p_head;
  int    methods;
  struct pattern_block *next;
};


struct subnet_block {
  struct IP_item *sub_list;
  struct pattern_block *pattern_list;
  char   *abort_log_name;
  char   *log_name;
  struct subnet_block *next;
};

/* What all this actually means:

    *subnet_head ----> cidr ----> cidr ----> cidr ...
	|\
	| \______> *pattern_file ----> *pattern_file ----> *pattern_file...
	|
	\/
    subnet_block ----> cidr...
	|\
	| \______> *pattern_file ----> *pattern_file
	|
	\/
    subnet_block ----> cidr ----> cidr ----> cidr ----> cidr...
	|\
	| \_______> *pattern_file...
	|
	\/
    subnet_block ----> cidr ----> cidr ----> cidr...
	.\
	. \_______> *pattern_file...
	.
	.

Now the various *pattern_files above point into the primary list below


    *pattern_head ----> pattern_item ----> pattern_item ...
	|
	|
	\/
     pattern_file ----> pattern_item...
	|
	|
	\/
     pattern_file ----> pattern_item....
	.
	.
	.

*/

void init_subnet_blocks(void);
void init_pattern_blocks(void);

struct subnet_block *add_subnet_block();

int add_to_ip_list(struct cidr, struct subnet_block *block);
int add_pattern_file(char *conf_filename, struct subnet_block *block, int methods);
void add_to_plist(struct REGEX_pattern, struct pattern_item **list);
void add_to_patterns(char *pattern, struct pattern_item **list);
void init_lists(void);
void free_lists(void);

#endif



