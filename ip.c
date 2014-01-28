/* $Id: ip.c,v 1.3 2005/08/21 10:44:06 chris Exp $ */

/*
  Squirm - A Redirector for Squid

  Maintained by Chris Foote, chris@foote.com.au
  Copyright (C) 1998/1999 Chris Foote

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "util.h"
#include "ip.h"
#include "paths.h"

/* stores address & subnet in a 'struct cidr'
   assumes:
   address is already in 1.2.3.4 format
   subnet is in the format /24

   If subnet is incorrect, returns 1
*/
int load_cidr(char *cidr, struct cidr *subnet)
{
  struct in_addr addr, mask, comp;
  int valid = true;

  char *address, *copy, *strtok_netmask, *destroyable;
  static char *netmask;
  static int netmask_is_allocated = false;

  if(netmask_is_allocated) {
    if(netmask != NULL)
      free(netmask);
    netmask_is_allocated = false;
  }

  copy = safe_strdup(cidr);
  if (copy == NULL) {
    return 1;
  }

  destroyable = copy;

  address = strtok(destroyable, "/");
  strtok_netmask = strtok(NULL, "/");
  if(strtok_netmask == NULL) {
    netmask = safe_strdup("32");
    netmask_is_allocated = true;
  } else {
    netmask = safe_strdup(strtok_netmask);
    netmask_is_allocated = true;
  }

  if(netmask == NULL) {
    netmask_is_allocated = false;
    return 1;
  }

  addr = get_ip_address(address, &valid);
  if(!valid)
    return 1;

  mask = get_netmask(netmask, &valid);
  if(!valid)
    return 1;

  /* we now do a bitwise AND to find out if the network
     address given is valid */
  comp.s_addr = mask.s_addr & addr.s_addr;
  if(comp.s_addr != addr.s_addr) {
    return 1;
  }

  subnet->address = addr;
  subnet->netmask = mask;
  free(copy);
  return 0;
}




/* convert CIDR subnet notation (e.g. /24) to a netmask
   as an in_addr
*/
struct in_addr get_netmask(char *netmask, int *valid)
{
  int mask_len;
  int bit, shift;
  struct in_addr or_bits;
  
  int mask_as_cidr;
  struct in_addr mask;
  mask.s_addr = -1;

  mask_len = strlen(netmask);
  if((mask_len < 1) || (mask_len > 2)) {
    *valid = false;
    return(mask);
  }

  mask_as_cidr = atoi(netmask);
  /* check the range is from a /0 to a /32 */
  if((mask_as_cidr < 0) || (mask_as_cidr > 32)) {
    *valid = false;
    return(mask);
  }
  
  /* we need to special case /0 */
  if(mask_as_cidr == 0) {
    mask.s_addr = 0;
    return(mask);
  }

  if(mask_as_cidr == 32) {
    mask.s_addr = 0xffffffff;
    return(mask);
  }

  /* convert CIDR to netmask */
  mask.s_addr = 0;
  for(bit = 1; bit <= mask_as_cidr; bit++) {
    shift = 32 - bit;
    or_bits.s_addr = 1 << shift;
    mask.s_addr |= or_bits.s_addr;
  }

  return(mask);
}




/* convert an IP address in dotted quad format (e.g. 1.2.3.4)
   to an unsigned long
*/
struct in_addr get_ip_address(char *address, int *valid)
{
  int stored, i;
  int octets[4];
  struct in_addr addr;
  addr.s_addr = -1;

  stored = sscanf(address, "%d.%d.%d.%d", &octets[0],&octets[1],
		  &octets[2],&octets[3]);
  if(stored != 4) {
    *valid = false;
    return(addr);
  }
  
  /* check that range of each quad is within 8 bits */
  for(i = 0; i < 4; i++) {
    if((octets[i] < 0) || (octets[i] > 255)) {
      *valid = false;
      return(addr);
    }
  }
  
  /* convert the dotted quad notation to an in_addr */
  addr.s_addr = 0;
  for(i = 0; i < 4; i++) {
    addr.s_addr |= octets[i];
    if(i != 3)
      addr.s_addr = addr.s_addr << 8;
  }
  
  return(addr);
}


/* Find out if the given IP address
   is within the range of the given subnet
   returns 1 if it does, and 0 otherwise

   uses pointers as arguments to save copy time
*/
int does_ip_match_subnet(struct cidr *subnet, struct in_addr *address)
{
  struct in_addr comp;

  comp.s_addr = subnet->netmask.s_addr & address -> s_addr;

  if(comp.s_addr == subnet->address.s_addr)
    return 1;

  return 0;
}


