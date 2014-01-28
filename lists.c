/* $Id: lists.c,v 1.3 2005/08/21 10:44:06 chris Exp $ */

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


#include"squirm.h"
#include"paths.h"
#include"log.h"
#include"lists.h"
#include"regex.h"
#include"config.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>


extern int dodo_mode;


void init_lists(void)
{
  subnet_head = NULL;
  pattern_head = NULL;
}



/* Initialises a block which represents a group of network prefixes in the conf file */
struct subnet_block *add_subnet_block()
{
  struct subnet_block *new;
  struct subnet_block *curr;

  new = NULL;
  curr = NULL;

  new = (struct subnet_block*)malloc(sizeof(struct subnet_block));
  if (new == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_subnet_block()\n");
    dodo_mode = 1;
    return NULL;
  }
 
  new->sub_list = NULL;
  new->pattern_list = NULL;
  new->abort_log_name = NULL;
  new->log_name = NULL;
  new->next = NULL;


  /* add it to the _end_ of the global list of subnets */
  if(subnet_head == NULL)
    subnet_head = new;
  else {
    for(curr = subnet_head; curr->next != NULL; curr = curr->next)
      ;
    curr->next = new;
  }
  
#ifdef DEBUG
  logprint(LOG_DEBUG, "created new subnet block\n");
#endif
  
  return new;  
}
/* end add_subnet_block */





int add_to_ip_list(struct cidr src_net, struct subnet_block *block)
{
  struct IP_item *curr;
  struct IP_item *new;

  curr = NULL;
  new = NULL;
  
  new = (struct IP_item *)malloc(sizeof(struct IP_item));
  if(new == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_to_ip_list()\n");
    dodo_mode = 1;
    return 1;
  }
  
  new->subnet = src_net;
  new->next = NULL;
  
  if(block->sub_list == NULL)
    block->sub_list = new;
  else {
    for(curr = block->sub_list; curr->next != NULL; curr = curr->next)
      ;
    curr->next = new;
  }

#ifdef DEBUG
  logprint(LOG_DEBUG, "inserted subnet %d %d\n", src_net.address.s_addr,
      src_net.netmask.s_addr);
#endif
  
  return 0;
}
/* end add_to_ip_list() */



int add_pattern_file(char *conf_filename, struct subnet_block *block,
		     int methods)
{
  struct pattern_file *p;
  struct pattern_block **curr;

  if (pattern_head != NULL) {
    for(p = pattern_head; p->next != NULL; p = p->next) {
      if (strcmp(conf_filename, p->filename) == 0) {
	/* we have a match, no need to build this list */
#ifdef DEBUG
	logprint(LOG_DEBUG, "Found already constructed pattern file\n");
#endif
	break;
      }
    }
    if ((p->next == NULL) && (strcmp(conf_filename, p->filename)!=0)) {
      /* we didn't find a match, must build the list, then make p point to it */
#ifdef DEBUG
	logprint(LOG_DEBUG, "Building pattern file [%s]\n", conf_filename);
#endif
        p->next = create_pattern_list(conf_filename);
      if (p->next == NULL) {
	logprint(LOG_ERROR, "unable to create_pattern_list() in add_pattern_file()\n");
	dodo_mode = 1;
	return -1;
      } 
      p = p->next;
    }
  } else {
      pattern_head = create_pattern_list(conf_filename);
      if (pattern_head == NULL) {
	logprint(LOG_ERROR, "unable to create_pattern_list() in add_pattern_file()\n");
	dodo_mode = 1;
	return -1;
      } 
      p = pattern_head;
  }

  /* now we have p pointing at the pattern file list we want, so insert */
  for (curr = &(block->pattern_list); *curr != NULL; curr = &((*curr)->next))
    ;
  /* insert the pattern_file on the end of the list */
  *curr = (struct pattern_block *)malloc(sizeof(struct pattern_block));
  if (*curr == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_pattern_file()\n");
    dodo_mode = 1;
    return -1;
  }
  (*curr)->p_head = p;
  (*curr)->next = NULL;
  (*curr)->methods = methods;

#ifdef DEBUG
  logprint(LOG_DEBUG, "inserted pattern file %s, with accept %d\n", conf_filename, methods);
#endif
  
  return 1;
}
/* end add_pattern_file */




void add_to_plist(struct REGEX_pattern pattern, struct pattern_item **list)
{
  struct pattern_item *curr;
  struct pattern_item *new;
  
  curr = NULL;
  new = NULL;
  
  /* two strings are already allocated in the "pattern" struct
     argument to this function */
  
  new = (struct pattern_item *)malloc(sizeof(struct pattern_item));
  if(new == NULL) {
    logprint(LOG_ERROR, "unable to allocate memory in add_to_plist()\n");
    /* exit(3); */
    dodo_mode = 1;
    return;
  }
  
  new->patterns.pattern = pattern.pattern;
  new->patterns.replacement = pattern.replacement;
  new->patterns.type = pattern.type;
  new->patterns.has_accel = pattern.has_accel;
  new->patterns.accel = pattern.accel;
  new->patterns.accel_type = pattern.accel_type;
  new->patterns.case_sensitive = pattern.case_sensitive;
  new->patterns.cpattern = pattern.cpattern;
  new->next = NULL;
  
  if(*list == NULL)
    *list = new;
  else {
    for(curr = *list; curr->next != NULL; curr = curr->next)
      ;
    curr->next = new;
  }

#ifdef DEBUG
  logprint(LOG_DEBUG, "inserted pattern item\n");
#endif
  
}



void free_pattern_blocks(struct pattern_block *head)
{
  struct pattern_block *prev;
  struct pattern_block *next;
  
  prev = NULL;
  next = NULL;
  
  
  if(head != NULL) {
    next = head->next;
    free(head);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed pattern block head\n");
#endif
  }
  
  for(prev = next; next != NULL; ) {
    next = prev->next;
    free(prev);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed pattern block\n");
#endif
    prev = next;
  }  
}






void free_ip_list(struct IP_item *head)
{
  struct IP_item *prev;
  struct IP_item *next;
  
  prev = NULL;
  next = NULL;
  
  
  if(head != NULL) {
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed ip list head\n");
#endif
    next = head->next;
    free(head);
  }
  
  for(prev = next; next != NULL; ) {
    next = prev->next;
    free(prev);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed ip list item\n");
#endif
    prev = next;
  }

}





void free_subnet_blocks(void)
{
  struct subnet_block *prev;
  struct subnet_block *next;
  
  prev = NULL;
  next = NULL;
  
  
  if(subnet_head != NULL) {
    next = subnet_head->next;
    if(subnet_head->abort_log_name)
      free(subnet_head->abort_log_name);
    if(subnet_head->log_name)
      free(subnet_head->log_name);
    free_ip_list(subnet_head->sub_list);
    free_pattern_blocks(subnet_head->pattern_list);
    free(subnet_head);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed subnet block head\n");
#endif
  }
  
  for(prev = next; next != NULL; ) {
    next = prev->next;
    if(prev->abort_log_name)
      free(prev->abort_log_name);
    if(prev->log_name)
      free(prev->log_name);
    free_ip_list(prev->sub_list);
    free_pattern_blocks(prev->pattern_list);
    free(prev);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed subnet block\n");
#endif
    prev = next;
  }
  
  subnet_head = NULL;
}



void free_plist(struct pattern_item *head)
{
  struct pattern_item *prev;
  struct pattern_item *next;
  
  prev = NULL;
  next = NULL;
  
  if(head != NULL) {
    next = head->next;
    if(head->patterns.pattern)
      free(head->patterns.pattern);
    if(head->patterns.replacement)
      free(head->patterns.replacement);
    if(head->patterns.accel)
      free(head->patterns.accel);
    free(head);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed plist item head\n");
#endif
  }
  
  for(prev = next; next != NULL; ) {
    next = prev->next;
    
    if(prev->patterns.pattern)
      free(prev->patterns.pattern);
    if(prev->patterns.replacement)
      free(prev->patterns.replacement);
    if(prev->patterns.accel)
      free(prev->patterns.accel);
    free(prev);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed plist item\n");
#endif
    prev = next;
  }
}







void free_pattern_files(void)
{
  struct pattern_file *prev;
  struct pattern_file *next;

  prev = NULL;
  next = NULL;

  if(pattern_head != NULL) {
    next = pattern_head->next;
    free_plist(pattern_head->head);
    if(pattern_head->filename)
      free(pattern_head->filename);
    free(pattern_head);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed pattern file head\n");
#endif
  }
  for(prev = next; next != NULL; ) {
    next = prev->next;
    free_plist(prev->head);
    if (prev->filename)
      free(prev->filename);
    free(prev);
#ifdef DEBUG
  logprint(LOG_DEBUG, "freed pattern file item\n");
#endif
    prev = next;
  }
  pattern_head = NULL;
}
   




void free_lists(void)
{
  free_subnet_blocks();
  free_pattern_files();
}
