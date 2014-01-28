/* $Id: ip.h,v 1.2 2005/08/19 07:31:06 chris Exp $ */

/*
  Squirm - A Redirector for Squid

  Maintained by Chris Foote, chris@foote.com.au
  Copyright (C) 1998-2005 Chris Foote

  If you find it useful, please let me know by sending
  email to chris@senet.com.au - Ta!
  
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


#include <netinet/in.h>

/* stores address and netmask in unsigned 32 bit variables (in_addr.s_addr) */
struct cidr {
  struct in_addr address;
  struct in_addr netmask;
};

#define false 0
#define true  1

/* loads a cidr structure from a "123.123.123.64/7" style string  */
int load_cidr(char *cidr , struct cidr *subnet);

/* normally for internal use */
struct in_addr get_netmask(char *netmask, int *valid);
struct in_addr get_ip_address(char *address, int *valid);

/* check whether an ip falls within a given cidr */
int does_ip_match_subnet(struct cidr *subnet, struct in_addr *address);


