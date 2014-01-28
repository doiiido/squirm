/* $Id: main.c,v 1.2 2005/08/19 07:31:06 chris Exp $ */
 
/*
  Squirm - A Redirector for Squid

  Maintained by Chris Foote, chris@foote.com.au
  Copyright (C) 1998-2005 Chris Foote

  If you find it useful, please let me know by sending
  email to chris@foote.com.au - Ta!
  
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


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/signal.h>
#include<ctype.h>
#include<time.h>

#include"paths.h"
#include"squirm.h"
#include"log.h"
#include"config.h"
#include"lists.h"



int dodo_mode = 0;
int first_run = 1;
int sig_hup = 0;
int interactive = 0;
char *alternate_config = NULL;

/*  exit values:
    0 = normal
    1 = file errors
    2 = abnormal errors
    3 = malloc problems
    
    Under normal conditions, Squirm will never abort (you don't
    want your Squid to suddenly die if you've put an invalid
    regex into your config file, or renamed the config file!  
    Under these abnormal conditions, squirm will run in dodo_mode 
    where stdin is echoed to stdout!

    If you've entered an invalid regex expression that regcomp()
    doesn't like, or perhaps an invalid network number, check it 
    out in the ERROR_LOG.
    
    It should never ever abort under normal circumstances - even
    if it can't allocate memory (which shouldn't happen, and Squid 
    would be running like a dog under low memory conditions anyway) 
    dodo_mode will be set and it will keep running :-)
    
    If squirm is running in dodo_mode, sending the redirector processes
    a HUP signal will cause squirm to try reading it's config files
    and building linked lists again.
*/



int main(int argc, char **argv)
{
  char buff[MAX_BUFF];
  char *redirect_url;
  struct IN_BUFF *in_buff = NULL;
  int finished = 0;
  int buff_status = 0;

  in_buff = (struct IN_BUFF *)malloc(sizeof(struct IN_BUFF));
  if (in_buff == NULL) {
    logprint(LOG_ERROR, "Couln't allocate memory in main()\n");
  }

  in_buff->url = (char *)malloc(MAX_BUFF);
  if (in_buff->url == NULL) {
    logprint(LOG_ERROR, "Couln't allocate memory in main()\n");
  }
  in_buff->src_address = (char *)malloc(MAX_BUFF);
  if (in_buff->src_address == NULL) {
    logprint(LOG_ERROR, "Couln't allocate memory in main()\n");
  }
  in_buff->ident = (char *)malloc(MAX_BUFF);
  if (in_buff->ident == NULL) {
    logprint(LOG_ERROR, "Couln't allocate memory in main()\n");
  }
  in_buff->method = (char *)malloc(MAX_BUFF);
  if (in_buff->method == NULL) {
    logprint(LOG_ERROR, "Couln't allocate memory in main()\n");
  }

  /* go into interactive mode if we're run as root */
  if((int)getuid() == 0) {
    interactive = 1;
    fprintf(stderr, "Squirm running as UID 0: writing logs to stderr\n");
  }

  /* check for alternate config file given as first argument */
  if(argc == 2)
    alternate_config = argv[1];

  if(argc > 2) {
    fprintf(stderr, "squirm:invalid arguments\n");
    fprintf(stderr, "Usage: %s [alternate-squirm-conf-file]\n", argv[0]);
    exit(1);
  }


  /*********************************
    main program loop, executed 
    forever more unless terminated
    by a kill signal or EOF on stdin 
    ********************************/
  while(! finished) {
    
    /* install the signal handler for re-reading config 
       files and freeing up linked lists     */
    signal(SIGHUP, squirm_HUP);
    sig_hup = 0;
    
    dodo_mode = 0;
    
    /* free old lists if we've been HUPped */
    if(! first_run) {
      logprint(LOG_INFO, "Freeing up old linked lists\n");
      free_lists();
    }


    /*********************
      read config files
      into linked lists
      
      We reread ALL files, because we destroyed our old lists just 
      above... to do it any way necessitates writing a basic garbage 
      collection routine...
      anyway, we shouldn't be hupped every 5 seconds anyway.
      ********************/

    if (alternate_config == NULL) {
      parse_squirm_conf(SQUIRM_CONF);
    } else {
      parse_squirm_conf(alternate_config);
    }

    if(dodo_mode)
      logprint(LOG_ERROR, "Invalid condition - continuing in DODO mode\n");
    
    logprint(LOG_INFO, "Squirm (PID %d) started\n", (int)getpid());


    while(!sig_hup && (fgets(buff, MAX_BUFF, stdin) != NULL)){
      
      /* if configs are completely invalid or some other
	 exception occurs where we want the redirector to
	 continue operation (so that Squid still works!),
	 we simply echo stdin to stdout - i.e. "dodo mode" :-) */

      if(dodo_mode) {
	puts("");
	fflush(stdout);
	continue;
      }


      /* separate the four fields
	from the single input line 
	of stdin */

      buff_status = load_in_buff(buff, in_buff);


      /* if four fields couldn't be separated, or the
	 converted values aren't appropriate, then
	 just echo back the line from stdin */

      if(buff_status == 1) {
	puts("");
	fflush(stdout);
	continue;
      }

      /* check dodo_mode again */
      if(dodo_mode) {
	puts("");
	fflush(stdout);
	logprint(LOG_ERROR, "Invalid condition - continuing in DODO mode\n");
	continue;
      }


      /* do the possible substitution */
      
      if((redirect_url = squirm_it(in_buff)) == NULL) {

	/* no replacement for the URL was found */
	puts("");
	fflush(stdout);

	continue;

      } else {

	/* redirect_url contains the replacement URL */
	printf("%s %s %s %s\n", redirect_url, in_buff->src_address,
	       in_buff->ident, in_buff->method);
	fflush(stdout);
	free(redirect_url);

	continue;
      }
      
    }

    first_run = 0;
    if(! sig_hup)
      finished = 1;
    
  } /* end while(!finished) */
  

  /* free lists and buffers */
  free_lists();
  free(in_buff->url);
  free(in_buff->ident);
  free(in_buff->method);
  free(in_buff->src_address);
  free(in_buff);

  return 0;
}











