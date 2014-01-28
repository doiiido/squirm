/* $Id: squirm.h,v 1.2 2005/08/19 07:31:06 chris Exp $ */

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


#ifndef SQUIRM_H

#ifndef LISTS_H
#include"lists.h"
#endif

#define SQUIRM_H 1

#include<stdarg.h>

#define SMALL_BUFF 256
#define MAX_BUFF 8000
#define true  1
#define false 0

struct IN_BUFF {
  char *url;
  char *src_address;
  char *ident;
  char *method;
};

int load_in_buff(char *, struct IN_BUFF *);

char *replace_string (struct pattern_item *, char *);
void squirm_HUP(int);
int match_accel(char *url, char *accel, int accel_type, int case_sensitive);
char *squirm_it(struct IN_BUFF *buff);
int count_parenthesis (char *);


/***** config ******/
int get_ip(char *buff, struct in_addr *src_address);
int load_patterns(struct pattern_item **list, char *conf_filename);
char *get_accel(char *accel, int *accel_type, int case_sensitive);
int makeup_accel(char *accel, const char *pattern);


#endif





















