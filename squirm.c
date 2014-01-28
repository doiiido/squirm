/* $Id: squirm.c,v 1.2 2005/08/19 07:31:06 chris Exp $ */
   
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
#include"config.h"
#include"util.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/signal.h>
#include<ctype.h>

/*
 * according to Jason Patterson <jason@pattosoft.com.au>,
 * USE_SYSTEM_STRING_FUNCS gives a ~10% speedup for
 * squirm overall on these platforms.
 */
#if defined(sparc) || defined(mips) || defined(powerpc) || defined(alpha)
#define USE_SYSTEM_STRING_FUNCS
#endif

extern int sig_hup;                        /* from main.c  */
extern struct subnet_block *subnet_head;   /* from lists.h */
extern struct pattern_file *pattern_head;  /* from lists.h */


/*
  load the stdin for the redirector into
  an IN_BUFF structure 
  
  Sets in_buff.url to "" if the fields can't
  be converted
*/
int load_in_buff(char *buff, struct IN_BUFF *in_buff)
{
  int converted;
  int len;
  char *tmp;
  
  strcpy(in_buff->url, "");
  strcpy(in_buff->src_address, "");
  strcpy(in_buff->ident, "");
  strcpy(in_buff->method, "");
  
  converted = sscanf(buff, "%s %s %s %s\n", in_buff->url, 
		     in_buff->src_address, in_buff->ident, in_buff->method);
  
  if(converted != 4) {
    logprint(LOG_ERROR, "%d = incorrect number of scanned fields\n", converted);
    return 1;
  }
  
  /* check the source IP address */
  if(strcmp(in_buff->src_address, "") == 0) {
    logprint(LOG_ERROR, "in_buff.src_address is NULL in main()\n");
    return 1;
  }

  /* handle the error condition in which 
     4 arguments not parsed, in which we just 
     print buff to stdout
  */
  len = strlen(in_buff->url);
  if(len <= 4) {
    logprint(LOG_FAIL, "strlen in_buff->url = [%d] in main()\n", len);
    return 1;
  }

  /* chop off /fqd */
  tmp = index(in_buff->src_address, '/');
  if (tmp == NULL) {
    /* there was no /fqd hmmm.. bad */
    logprint(LOG_ERROR, "Input incorrectly formed - in ip/fqdn - there was no /-");
    return 1;
  }
  *tmp = '\0';

  return 0;
}




/* match_method() returns 1 on match, 0 for no-match */
int match_method(char *method, int accept) {

  lower_case(method);

  /* match all first */
  if (accept == ALL) {
    return 1;
  }

  /* and then the most common */
  if (strcmp(method,"get") == 0) {
    return ((GET & accept) != 0 ? 1 : 0); 
  }
  if (strcmp(method,"put") == 0) {
    return ((PUT & accept) != 0 ? 1 : 0); 
  }
  if (strcmp(method,"post") == 0) {
    return ((POST & accept) != 0 ? 1 : 0); 
  }
  if (strcmp(method,"head") == 0) {
    return ((HEAD & accept) != 0 ? 1 : 0); 
  }
  return 0;
}




/* compare_ip() returns 1 on match, 0 for no-match
   this runs down the list of cidr to possibly find a match
*/
int compare_ip(struct in_addr *address, struct subnet_block *block)
{
  struct IP_item *curr;
  curr = NULL;
  
  for(curr = block->sub_list; curr != NULL; curr = curr->next)
    if(does_ip_match_subnet(&(curr->subnet), address))
      return 1;
  
  return 0;
}




void squirm_HUP(int signal)
{
  /* exciting bit of code this one :-) */
  sig_hup = 1;
  logprint(LOG_INFO, "sigHUP received.  Reconfiguring....\n");
}




/* returns replacement URL for a match, 
   NULL if no match found, 
   "" if it matches an abort
*/
char *pattern_compare(char *url, struct pattern_item *head)
{
  struct pattern_item *curr;
  char *new_string;
  int pos;
  int len;
  int i;
  int matched;
  int abort_on_match = 1;
  int url_changed = 0;
  curr = NULL;
  

  for(curr = head; curr != NULL; curr = curr->next) {
    
    /* return "" if string matches 
       ABORT file extension */
    
    matched = 1; /* assume a match until a character isn't the same */
    
    if(curr->patterns.type == ABORT) {
      len = strlen(curr->patterns.pattern);
      pos = strlen(url) - len;
      
      for(i = 0; i <= len; i++)
	if (url[pos+i] != curr->patterns.pattern[i]) {
	  matched = 0;
	  break;
	}
      
      if(matched)
	return ""; /* URL matches abort file extension */

    } else if (curr->patterns.type == ABORT_NORMAL) {
      /* check for accelerator string */
      if(curr->patterns.has_accel) {
	/* check to see if the accelerator string matches, 
	   then bother doing a regexec() on it */
	if(match_accel(url, curr->patterns.accel, 
		       curr->patterns.accel_type, 
		       curr->patterns.case_sensitive)) {

	  if(regexec(&curr->patterns.cpattern, url, 0, 0, 0) == 0) {
	    return ""; /* URL matches abort regular expression */
	  }
	} else {
	  /* no accelerator */
	  if(regexec(&curr->patterns.cpattern, url, 0, 0, 0) == 0) {
	    return ""; /* URL matches abort regular expression */
	  }
	}	  
      }
    } else if (curr->patterns.type == ABORT_EXTENDED) {
      /* check for accelerator string */
      if(curr->patterns.has_accel) {
	/* check to see if the accelerator string matches, 
	   then bother doing a regexec() on it
	*/
	if(match_accel(url, curr->patterns.accel, 
		       curr->patterns.accel_type, 
		       curr->patterns.case_sensitive)) {

	  if (replace_string (curr, url) != NULL) {
	    return ""; /* URL matches abort regular expression */
	  }
	} else {
	  /* no accelerator */
	  if (replace_string (curr, url) != NULL) {
	    return ""; /* URL matches abort regular expression */
	  }
	}
      }
    } else if (curr->patterns.type == ABORT_ON_MATCH) {
      
      if (strcmp(curr->patterns.pattern, "on") == 0)
        abort_on_match = 1;
      else
        abort_on_match = 0;
      
    } else {
      
      /* check for accelerator string */
      if(curr->patterns.has_accel) {
	
	/* check to see if the accelerator string matches, then bother
	   doing a regexec() on it */
	if(match_accel(url, curr->patterns.accel, 
		       curr->patterns.accel_type, 
		       curr->patterns.case_sensitive)) {
	  
	  /* Now we must test for normal or extended */
	  if (curr->patterns.type == EXTENDED) {
	    new_string = replace_string (curr, url);
	    if (new_string != NULL) {
	      if (abort_on_match) return (new_string);
	      else {
		url = new_string;
		url_changed = 1;
	      }
	    }
	  } else /* Type == NORMAL */ {
	    if(regexec(&curr->patterns.cpattern, url, 0, 0, 0) == 0) {
	      new_string = safe_strdup(curr->patterns.replacement);
	      if (abort_on_match) return (new_string);
	      else {
		url = new_string;
		url_changed = 1;
	      }
	    }
	  }
	} /* end match_accel loop */
      } else {
	/* we haven't got an accelerator string, so we use regex
	   instead */
	
	/* Now we must test for normal or extended */
	if (curr->patterns.type == EXTENDED) {
	  new_string = replace_string (curr, url);
	  if (new_string != NULL) {
	    if (abort_on_match) return (new_string);
	    else {
	      url = new_string;
	      url_changed = 1;
	    }
	  }
	} else /* Type == NORMAL */ {
	  if(regexec(&curr->patterns.cpattern, url, 0, 0, 0) == 0) {
	    new_string = safe_strdup(curr->patterns.replacement);
	    if (abort_on_match) return (new_string);
	    else {
	      url = new_string;
	      url_changed = 1;
            }
	  }
	}
      }
    }
  }
  
  /* return NULL if no match */
  /* "" string would have been returned in an abort */
  return (url_changed ? url : NULL);
}





/* 
   iterate through lists checking ip until correct ranges found
   and then iterating down through pattern lists doing possible substitutions
   
   abort on match will only work in the one pattern file
*/

char *squirm_it(struct IN_BUFF *buff)
{
  struct subnet_block *sb;
  struct in_addr address;
  char *mashed;
  struct pattern_block *pb;
  int valid = true;

  /* parse the address */
  address = get_ip_address(buff->src_address, &valid);

  if(!valid) {
    logprint(LOG_ERROR, "source address invalid or not parsed correctly\n");
    return NULL;
  }


  /* iterate over the network blocks */
  for (sb = subnet_head; sb != NULL; sb = sb->next) {

    /* let compare_ip run down the list of cidr in 
       that block to check for match
    */

    if (compare_ip(&address, sb) == 1) {

      /* if we got a match, then we run down the list 
	 of pattern files present in that block of cidr
      */
      for (pb = sb->pattern_list; pb != NULL; pb = pb->next) {
	/* and then we see if the method matches the pattern block
	   NB that methods are associated with blocks, not files
	*/

	if (match_method(buff->method, pb->methods) == 1) {
	  /* and now we check the pattern block */
	  mashed = pattern_compare(buff->url, pb->p_head->head);
	  /* if we get "" it was an abort, 
	     NULL is no match, 
	     "..." is a replacement
	  */
	  if (mashed != NULL) {
	    if (strcmp(mashed, "") != 0) {
	      /* we have a string with replacement */
	      if(sb->log_name != NULL) {
		/* possibly log to match log */
		logprint(sb->log_name, "%s:%s\n", buff->url, mashed);
	      }
	      return mashed;
	    } else {
	      /* we have an abort */
	      if(sb->abort_log_name != NULL) {
		/* possibly log to abort log */
		logprint(sb->abort_log_name, "%s\n", buff->url);
	      }
	      /* an abort with no log */
	      return NULL;
	    }
	  }
	}
      }
    }
  }
  /* no match throughout */
  return NULL;
}





/* see if the url matches an accelerator */
int match_accel(char *url, char *accel, int accel_type, int case_sensitive)
{
  /* return 1 if url contains accel */
  int i, offset;
  char l_url[MAX_BUFF];
  int accel_len;
  int url_len;
  
  /* an accelerator that appears in the middle */  
  if(accel_type == ACCEL_NORMAL) {
    if(case_sensitive) {
      for(i = 0; url[i] != '\0'; i++)
	l_url[i] = url[i];
      l_url[i] = '\0';
    } else {
      /* convert to lower case */
      for(i = 0; url[i] != '\0'; i++)
	l_url[i] = tolower(url[i]);
      l_url[i] = '\0';
    }
    
    if(strstr(l_url, accel)) {
      return 1;
    } else {
      return 0;
    }
  }
  
  
  /* an accelerator which is text that appears at the start */
  if(accel_type == ACCEL_START) {
    accel_len = strlen(accel);
    url_len = strlen(url);
    if(url_len < accel_len)
      return 0;
    
    if(case_sensitive) {
      #ifdef USE_SYSTEM_STRING_FUNCS
      return !strncmp(url, accel, accel_len);
      #else
      for(i = 0; i < accel_len; i++) {
	if(accel[i] != url[i])
	  return 0;
      }
      #endif
    } else {
      #ifdef USE_SYSTEM_STRING_FUNCS
      return !strncasecmp(url, accel, accel_len);
      #else
      for(i = 0; i < accel_len; i++) {
	if(accel[i] != tolower(url[i]))
	  return 0;
      }
      #endif
    }
    return 1;
  }
  

  /* an accelerator which is text that appears at the end */
  if(accel_type == ACCEL_END) {
    accel_len = strlen(accel);
    url_len = strlen(url);
    offset = url_len - accel_len;
    
    if(offset < 0)
      return 0;
    
    if(case_sensitive) {
      #ifdef USE_SYSTEM_STRING_FUNCS
      return !strncmp(url+offset, accel, accel_len);
      #else
      for(i = 0; i < accel_len; i++) {
	if(accel[i] != url[i+offset])
	  return 0;
      }
      #endif
    } else {
      #ifdef USE_SYSTEM_STRING_FUNCS
      return !strncasecmp(url+offset, accel, accel_len);
      #else
      for(i = 0; i < accel_len; i++) {
	if(accel[i] != tolower(url[i+offset]))
	  return 0;
      }
      #endif
    }
    return 1;
  }
  
  /* we shouldn't reach this section! */
  return 0;
}






char *replace_string (struct pattern_item *curr, char *url)
{
  char buffer[MAX_BUFF];
  char *replacement_string = NULL;
  regmatch_t match_data[10];
  int parenthesis;
  char *in_ptr;
  char *out_ptr;
  int replay_num;
  int count;
  
  
  /* Perform the regex call */
  if (regexec (&(curr->patterns.cpattern), url, 10, &match_data[0], 0) != 0)
    return (NULL);
  
  /* setup the traversal pointers */
  in_ptr = curr->patterns.replacement;
  out_ptr = buffer;
  
  /* Count the number of replays in the pattern */
  parenthesis = count_parenthesis (curr->patterns.pattern);

  if (parenthesis < 0) {
    /* Invalid return value - don't log because we already have done it */
    return (NULL);
  }
  
  /* Traverse the url string now */
  while (*in_ptr != '\0') {
    if (isdigit (*in_ptr)) {
      /* We have a number, how many chars are there before us? */
      switch (in_ptr - curr->patterns.replacement) {
      case 0:
	/* This is the first char
	   Since there is no backslash before hand, this is not
	   a pattern match, so loop around
	*/
	{
	  *out_ptr = *in_ptr;
	  out_ptr++;
	  in_ptr++;
	  continue;
	}
      break;
      case 1:
	/* Only one char back to check, so see if it is a backslash */
	if (*(in_ptr - 1) != '\\') {
	  *out_ptr = *in_ptr;
	  out_ptr++;
	  in_ptr++;
	  continue;
	}
	break;
      default:
	/* Two or more chars back to check, so see if the previous is
	   a backslash, and also the one before. Two backslashes mean
	   that we should not replace anything! */
	if ( (*(in_ptr - 1) != '\\') ||
	     ((*(in_ptr - 1) == '\\') && (*(in_ptr - 2) == '\\')) ) {
	  *out_ptr = *in_ptr;
	  out_ptr++;
	  in_ptr++;
	  continue;
	}
      }
      
      /* Ok, if we reach this point, then we have found something to
	 replace. It also means that the last time we went through here,
	 we copied in a backslash char, so we should backtrack one on
	 the output string before continuing */
      out_ptr--;
      
      /* We need to convert the current in_ptr into a number for array
	 lookups */
      replay_num = (*in_ptr) - '0';
      
      /* Now copy in the chars from the replay string */
      for (count = match_data[replay_num].rm_so; 
	   count < match_data[replay_num].rm_eo; count++) {
	/* Copy in the chars */
	*out_ptr = url[count];
	out_ptr++;
      }
      
      /* Increment the in pointer */
      in_ptr++;
    } else {
      *out_ptr = *in_ptr;
      out_ptr++;
      in_ptr++;
    }
    
    
    /* Increment the in pointer and loop around */
    /* in_ptr++; */
  }
  
  /* Terminate the string */
  *out_ptr = '\0';
  
  /* The return string is done, return to the caller */
  replacement_string = safe_strdup(buffer);
  return (replacement_string);
}





int count_parenthesis (char *pattern)
{
  int lcount = 0;
  int rcount = 0;
  int i;
  
  /* Traverse string looking for ( and ) */
  for (i = 0; i < strlen (pattern); i++) {
    /* We have found a left ( */
    if (pattern [i] == '\(') {
      /* Do not count if there is a backslash */
      if ((i != 0) && (pattern [i-1] == '\\'))
	continue;
      else
	lcount++;
    }
      
    if (pattern [i] == ')') {
      if ((i != 0) && (pattern [i-1] == '\\'))
	continue;
      else
	rcount++;
    }
  }
  
  /* Check that left == right */
  if (lcount != rcount)
    return (-1);
  
  return (lcount);
}
