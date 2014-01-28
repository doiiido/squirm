/* $Id: config.c,v 1.3 2005/08/21 03:07:32 chris Exp $ */

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
#include"lists.h"
#include"config.h"
#include"util.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<time.h>

extern int dodo_mode;            /* from main.c */
extern char *alternate_config;   /* from main.c */


#define min(a,b) ((a)<(b) ? (a) : (b))


/* returns a point to the first non whitespace character in string */
char *trim_leading_white_space(char *input)
{
  while ((*input != '\0') && isspace(*input)) {
    input++;
  }
  return input;
}


/* case insensitive, leading white space insensitive check
   returns 1 if it does start with, 0 if not */

int starts_with(char *haystack, char *needle)
{
  haystack = trim_leading_white_space(haystack);
  return (strncasecmp(haystack, needle, strlen(needle)) ? 0 : 1);
}


/* returns the stuff after an initial keyword */
char *get_stuff_after_keyword(char *input)
{
  char *tmp;
  /* clear off the first word */
  for (tmp = trim_leading_white_space(input); (*tmp != '\0') && !isspace(*tmp) ; tmp++)
    ;
  return trim_leading_white_space(tmp);
}
  

/* returns a fully qualified file name
   input filenames can be specified one of three way:
     relative to the squirm base directory eg. logs/abort.log => PREFIX/logs/abort.log

     as a plain name, in which case it gets put in the appropriate place
     eg. abort.log => PREFIX/(dir)/abort.log

     or as a fully qualified name.

     so relative have a slash in them, but not a leading slash
     plain have no slash, and fully qualified have a leading slash.

*/

char *gen_fq_name(char *name, char *dir)
{
  char *result;

  if (starts_with(name, "/")) {
    /* we have a fully qualified name */
    result = safe_strdup(name);
    return result; /* the caller will take care of null return */
  }

  /* must be a plain name */
  result = (char *)malloc(strlen(dir) + strlen(name) + 2);
  if (result) {
    sprintf(result, "%s/%s", dir, name);
  }
  return result;
}



/* parse the squirm.conf file
   returns 1 on success, 0 on failure
   */
int parse_squirm_conf(char *filename)
{
  FILE *fp;
  char buff[MAX_BUFF];
  struct cidr src_cidr;
  char *tmp1, *tmp2, *pattern_filename, *fq_pattern_filename;
  struct subnet_block *curr_block;
  int methods;

  pattern_filename = NULL;
  curr_block = NULL;
  fp = fopen(filename, "rt");
  init_lists();

  if(fp == NULL) {
    logprint(LOG_ERROR, "unable to open configuration file [%s]\n", filename);
    dodo_mode = 1;
    return 0;
  }

  logprint(LOG_INFO, "processing configuration file [%s]\n", filename);
  
  while(!dodo_mode && (fgets(buff, MAX_BUFF, fp) != NULL)) {
    
    /* skip blank lines and comments */
    if((strncmp(buff, "#", 1) == 0) || (strncmp(buff, "\n", 1) == 0))
      continue;
    
    if(strlen(buff) != 1) {
      /* chop newline */
      buff[strlen(buff) - 1] = '\0';
    
      /* do our stuff with the line */
      /* we can have the following appear
	 
	 begin
	 network
	 log
	 abort-log
	 pattern
	 end

	 */


      /* begin */
      if(starts_with(buff, "begin") == 1) {
	if (curr_block != NULL) {
	  logprint(LOG_ERROR, "begin keyword found within block in %s\n", filename);
	  dodo_mode = 1;
	  return 0;
	} else {
	  curr_block = add_subnet_block();
	}
	continue;
      }


      /* network */
      if (starts_with(buff, "network") == 1) {
	if (curr_block == NULL) {
	  logprint(LOG_ERROR, "network keyword found outside of block in  %s\n", filename);
	  dodo_mode = 1;
	  return 0;
	} else {
	  /* we need to get the network cidr from after the keyword network */
	  if(load_cidr(strpbrk(buff, "0123456789"), &src_cidr)) {
	    logprint(LOG_ERROR, "Invalid IP network [%s] in config file\n", buff);
            dodo_mode = 1;
            return 0;
	  } else {
	    add_to_ip_list(src_cidr, curr_block);
	  }
      
	}
	continue;
      }

     
      /* check for an optional abort-log file name */
      if (starts_with(buff, "abort-log") == 1) {
	if (curr_block == NULL) {
	  logprint(LOG_ERROR, "abort-log keyword found outside of block in %s\n", filename);
	  dodo_mode = 1;
	  return 0;
	} else {
	  curr_block->abort_log_name = gen_fq_name(get_stuff_after_keyword(buff), LOGDIR);
	  if (curr_block->abort_log_name == NULL) {
	    logprint(LOG_ERROR, "couldn't allocate memory in parse_squirm_conf()\n");
	    dodo_mode = 1;
	    return 0;
	  }
	}
	continue;
      }


      /* check for an optional log file name (logs matches) */
      if (starts_with(buff, "log")) {
	if (curr_block == NULL) {
	  logprint(LOG_ERROR, "log keyword found outside of block in %s\n", filename);
	  dodo_mode = 1;
	  return 0;
	} else {
	  curr_block->log_name = gen_fq_name(get_stuff_after_keyword(buff), LOGDIR);
	  if (curr_block->log_name == NULL) {
	    logprint(LOG_ERROR, "couldn't allocate memory in parse_squirm_conf()\n");
	    dodo_mode = 1;
	    return 0;
	  }
	}
	continue;
      }
      


      /* checking for a pattern filename */
      if (starts_with(buff, "pattern") == 1) {
	if (curr_block == NULL) {
	  logprint(LOG_ERROR, "pattern keyword found outside of block in %s\n", filename);
	  dodo_mode = 1;
	  return -1;
	} else {
	  /* format is pattern filename method[, method...] */  
	  tmp1 = get_stuff_after_keyword (buff);
	 
 	  /* find where white space begins again  */
	  tmp2 = index(tmp1, ' ');
	  if (tmp2 == NULL) {
	    /* no white space at end, but there must be white space between the
	       filename and obligatory method */
	    logprint(LOG_ERROR, "error parsing pattern file name in %s\n", filename);
	    dodo_mode = 1;
	    return -1;
	  } else {
	    pattern_filename = (char *)malloc((tmp2 - tmp1) * sizeof(char)+1);
	    strncpy(pattern_filename, tmp1, tmp2-tmp1);
	    pattern_filename[tmp2-tmp1] = '\0';
	  }
	  if (pattern_filename == NULL) {
	    logprint(LOG_ERROR, "couldn't allocate memory in parse_squirm_conf()\n");
	    dodo_mode = 1;
	    return 0;
	  }

	  fq_pattern_filename = gen_fq_name(pattern_filename, ETCDIR);
	  if (fq_pattern_filename == NULL) {
	    logprint(LOG_ERROR, "couldn't allocate memory in parse_squirm_conf()\n");
	    dodo_mode = 1;
	    return 0;
	  }

	  free(pattern_filename);

	  /* now we start looking for methods */
	  tmp1 = tmp2;
	  methods = 0;
	  while (tmp1 != NULL) {
	    tmp1 = trim_leading_white_space(tmp1);
	    if (tmp1 == NULL) {
	      break;
	    }
	    if (strncasecmp(tmp1, "get", min(strlen("get"), strlen(tmp1))) == 0) {
	      methods |= GET;
	      tmp1 += strlen("get");
	    }
	    else if (strncasecmp(tmp1,"put", min(strlen("put"), strlen(tmp1))) == 0) {
	      methods |= PUT;
	      tmp1 += strlen("put");
	    }
	    else if (strncasecmp(tmp1,"post", min(strlen("post"), strlen(tmp1))) == 0) {
	      methods |= POST;
	      tmp1 += strlen("post");
	    }
	    else if (strncasecmp(tmp1, "head", min(strlen("head"), strlen(tmp1))) == 0) {
	      methods |= HEAD;
	      tmp1 += strlen("head");
	    }
	    else if (strncasecmp(tmp1, "all", min(strlen("all"), strlen(tmp1))) == 0) {
	      methods |= ALL;
	      tmp1 += strlen("all");	      
	    }
	    
	    tmp1 = index(tmp1, ',');
	    if (tmp1 != NULL) {
	      tmp1++;
	    }
	  }

	  /* insert all the information */
	  if (add_pattern_file(fq_pattern_filename, curr_block, methods) == -1) {
	    logprint(LOG_ERROR, "couldn't parse pattern file %s\n", fq_pattern_filename);
	    dodo_mode = 1;
	    return 0;
	  }
	  free(fq_pattern_filename);
	}

	continue;
      }

      /* check for the presence of an end */
      if (strstr(buff, "end") != NULL) {
	if (curr_block == NULL) {
	  logprint(LOG_ERROR, "found keyword end outside of block in file %s\n", filename);
	  dodo_mode = 1;
	  return -1;
	} else {
	  curr_block = NULL;
	}
	continue;
      }	
      
      /* oops... not a valid option */
      logprint(LOG_ERROR, "found garbage [%s] in configuration file [%s]\n", buff, filename);
    } 
  }
  
  fclose(fp);
  return 1;
}



/* starts the parsing of  pattern files as called in parse_squirm_conf
   creates a pattern_file, which includes a list of all patterns in conf_filename */
struct pattern_file *create_pattern_list (char *conf_filename)
{
  struct pattern_file *tmp;

  tmp = (struct pattern_file *)malloc(sizeof(struct pattern_file));
  if (tmp == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in create_pattern_list()\n");
    dodo_mode = 1;
    return NULL;
  }
  
  tmp->head = NULL;
  tmp->filename = safe_strdup(conf_filename);
  if (tmp->filename == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in create_pattern_list()\n");
    dodo_mode = 1;
    free(tmp);
    return NULL;
  }
  tmp->next = NULL;

  if (load_patterns(&(tmp->head), conf_filename) == -1) {
    return NULL;
  }

  return tmp;
}



/* load the squirm.patterns into
   linked list */

int load_patterns(struct pattern_item **list, char *conf_filename)
{
  char buff[MAX_BUFF];
  FILE *fp;

  fp = fopen(conf_filename, "rt");

  if(fp == NULL) {
    logprint(LOG_ERROR, "unable to open patterns file [%s]\n", conf_filename);
    dodo_mode = 1;
    return -1;
  }

  logprint(LOG_INFO, "Reading patterns from file %s\n", conf_filename);
  
  while(!dodo_mode && (fgets(buff, MAX_BUFF, fp) != NULL)) {
    
    /* skip blank lines and comments */
    if((strncmp(buff, "#", 1) == 0) || (strncmp(buff, "\n", 1) == 0))
      continue;
    
    if(strlen(buff) != 1) {
      /* chop newline */
      buff[strlen(buff) - 1] = '\0';
      add_to_patterns(buff, list);
    }
  }  
  
  fclose(fp);
  return 1;
}




/* actually compiles individual regex etc. */
void add_to_patterns(char *pattern, struct pattern_item **list)
{
  char first[MAX_BUFF];
  char second[MAX_BUFF];
  char type[MAX_BUFF];
  char accel[MAX_BUFF];
  regex_t compiled;
  struct REGEX_pattern rpattern;
  int abort_type = 0;
  int abort_regex_type = 0;
  int parenthesis;
  int stored;
  
  /*  The regex_flags that we use are:
      REG_EXTENDED 
      REG_NOSUB 
      REG_ICASE; */

  int regex_flags = REG_NOSUB;
  
  rpattern.type = NORMAL;
  rpattern.case_sensitive = 1;
  
  stored = sscanf(pattern, "%s %s %s %s", type, first, second, accel);
  
  
  if((stored < 2) || (stored > 4)) {
    logprint(LOG_ERROR, 
	"unable to get a pair of patterns in add_to_patterns() "
	"for [%s]\n", pattern);
    dodo_mode = 1;
    return;
  }
  
  if(stored == 2)
    strcpy(second, "");

  /* type to lower case so as to be case insensitive */
  lower_case(type);
  
  if(strcmp(type, "abort") == 0) {
    rpattern.type = ABORT;
    abort_type = 1;
  }

  if(strcmp(type, "abortregexi") == 0) {
    abort_regex_type = 1;
    regex_flags |= REG_ICASE;
    rpattern.case_sensitive = 0;
    rpattern.type = ABORT_NORMAL;
  }

  if(strcmp(type, "abortregex") == 0) {
     abort_regex_type = 1;
     rpattern.type = ABORT_NORMAL;
  }
  
  if (strcmp(type, "abort_on_match") == 0) {
    rpattern.type = ABORT_ON_MATCH;
    abort_type = 1;
    /* make sure on or off is in the correct state */
    lower_case (first);
  }
  
  if(strcmp(type, "regexi") == 0) {
    regex_flags |= REG_ICASE;
    rpattern.case_sensitive = 0;
  }

  if(!abort_type) {
    
    parenthesis = count_parenthesis (first);
    
    if (parenthesis < 0) {
      
      /* The function returned an invalid result, 
	 indicating an invalid string */
      
      logprint(LOG_ERROR, "count_parenthesis() returned "
	   "left count did not match right count for line: [%s]\n",
	   pattern);
      dodo_mode = 1;
      return;

    } else if (parenthesis > 0) {
      regex_flags |= REG_EXTENDED;
      regex_flags ^= REG_NOSUB;
      if (!abort_regex_type) {
	rpattern.type = EXTENDED;
      } else {
	rpattern.type = ABORT_EXTENDED;
      }
    }
  }
  

  /* compile the regex */
  if(regcomp(&compiled, first, regex_flags)) {
    logprint(LOG_ERROR, "Invalid regex [%s] in pattern file\n", first);
    dodo_mode = 1;
    return;
  }
  
  
  /* put the compiled regex into the structure, along with replacement etc. */
  rpattern.cpattern = compiled;
  rpattern.pattern = (char *)malloc(sizeof(char) * (strlen(first) +1));
  if(rpattern.pattern == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_to_patterns()\n");
    dodo_mode = 1;
    return;
  }
  strcpy(rpattern.pattern, first);
  

  rpattern.replacement = (char *)malloc(sizeof(char) * (strlen(second) +1));
  if(rpattern.replacement == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_to_patterns()\n");
    dodo_mode = 1;
    return;
  }
  strcpy(rpattern.replacement, second);
  
  
  /* make up an accel string if one was not supplied */
  /* if we have a regex type */
  if (!abort_type) {
    /* if it is a abort regex, 3 fields means that we have accelerator */
    if (abort_regex_type && (stored != 3)) {
      makeup_accel(accel, first);
      stored = 3;
    }

    /* if it normal replacement regex, 4 fields means we have an accelerator already */
    if (!abort_regex_type && (stored != 4)) {
      makeup_accel(accel, first);
      stored = 4;
    }
  }      


  if (accel != NULL) {
    rpattern.has_accel = 1;
    rpattern.accel = get_accel(accel, &rpattern.accel_type, 
			       rpattern.case_sensitive);
    
    if(rpattern.accel == NULL) {
      logprint(LOG_ERROR, "unable to allocate memory from get_accel()\n");
      dodo_mode = 1;
      return;
    }
  } else {
    rpattern.has_accel = 0;
    rpattern.accel = NULL;
  }
  
/* finally, add it to the end of the list */
add_to_plist(rpattern, list);
}





char *get_accel(char *accel, int *accel_type, int case_sensitive)
{
  /* returns the stripped accelerator string or NULL 
     if memory can't be allocated 

     converts the accel string to lower case 
     if(case_sensitive) */

  /* accel_type is assigned one of the values:
     #define ACCEL_NORMAL 1
     #define ACCEL_START  2
     #define ACCEL_END    3     */
  
  int len, i;
  char *new_accel = NULL;
  *accel_type = 0;
  
  
  len = strlen(accel);
  if(accel[0] == '^')
    *accel_type = ACCEL_START;
  if(accel[len - 1] == '$')
    *accel_type = ACCEL_END;
  if(! *accel_type)
    *accel_type = ACCEL_NORMAL;
  
  if(*accel_type == ACCEL_START || *accel_type == ACCEL_END) {
    
    /* copy the strings */
    new_accel = (char *)malloc(sizeof(char) * strlen(accel));
    if(new_accel == NULL)
      return NULL;
    
    if(*accel_type == ACCEL_START) {
      if(case_sensitive)
	for(i = 0; i < len; i++)
	  new_accel[i] = accel[i+1];
      else
	for(i = 0; i < len; i++)
	  new_accel[i] = tolower(accel[i+1]);
    }

    if(*accel_type == ACCEL_END) {
      if(case_sensitive)
	for(i = 0; i < len - 1; i++)
	  new_accel[i] = accel[i];
      else
	for(i = 0; i < len - 1; i++)
	  new_accel[i] = tolower(accel[i]);
      new_accel[i] = '\0';
    }
    
  } else {

    new_accel = safe_strdup(accel);

    if(!case_sensitive) {
      for(i = 0; i < len; i++)
	new_accel[i] = tolower(accel[i]);
      new_accel[i] = '\0';
    }

  }

  
  return new_accel;
}




/* generates an accelerator from the regex */
int makeup_accel(char *accel, const char *pattern)
{
  int accel_len = 0;
  const char *p = pattern;
  char tmp[MAX_BUFF];
  int i, done;
  char br;
  
  while (*p)
  {
    /* gather the next sequence of constant characters */
    for (i=0, done=0; !done && *p; ++p)
    {
      switch (*p)
      {
       case '.': case '*': case '+': case '?': case '\0':
        done = 1;
        break;
        
       case '(': case '[': case '{':
        done = 1;
        /* skip to closing bracket */
        br = (*p) + ((*p == '(') ? 1 : 2);  /* assumes ASCII */
        while (*p != br && *p) ++p;
        break;
        
       case '\\':
        if (*(p+1)) { ++p; tmp[i++] = *p; }
        else done = 1;
        break;

       default:  /* a normal character */
        tmp[i++] = *p;
        break;
      }
    }
    tmp[i] = '\0';
    
    /* pick it if longer than the others and non-useless */
    if (i > accel_len && strcmp(tmp, "^http://") != 0)
    { strcpy(accel, tmp); accel_len = i; }
  }

  /* dismiss meaningless cases */
  if (accel_len == 1 && (accel[0] == '^' || accel[0] == '$'))
    accel_len = 0;
  
  if (accel_len) logprint(LOG_INFO, "Generated accelerator %s from %s\n", accel, pattern);
  
  return accel_len;
}



















