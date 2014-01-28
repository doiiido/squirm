/* $Id: log.c,v 1.2 2005/08/19 07:31:06 chris Exp $ */

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


#include"paths.h"
#include"squirm.h"
#include"log.h"
#include<stdio.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>

extern int interactive;       /* from main.c */



void logprint(char *filename, char *format, ...)
{
  FILE *log;
  char *date_str = NULL;
  char msg[MAX_BUFF];
  va_list args;
  
  va_start(args, format);
  if(vsprintf(msg, format, args) > (MAX_BUFF - 1)) {
    /* string is longer than the maximum buffer we 
       specified, so just return */
    return;
  }
  va_end(args);

  if(interactive) {
    date_str = get_date();
    fprintf(stderr, "%s:%s", date_str, msg);
    fflush(stderr);
    free(date_str);
  } else {
    log = fopen(filename, "at");
    
    if(log == NULL)
      return;
  
    date_str = get_date();
    
    fprintf(log, "%s:%s", date_str, msg);
    fflush(log);
    free(date_str);
    fclose(log);
  }
}


char *get_date(void)
{
  time_t tp;
  char *ascitime;
  char *s;
  
  tp = time(NULL);
  
  ascitime = ctime(&tp);
  
  s = (char *)malloc(sizeof(char) * (strlen(ascitime)+1));
  if(s == NULL)
    exit(3); /* no use writing an error message, because this function
                will keep getting called! */
  strcpy(s, ascitime);
  s[strlen(ascitime) - 1] = '\0';
  
  return s;
}


















