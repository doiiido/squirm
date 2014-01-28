/* $Id: paths.h,v 1.2 2005/08/19 07:31:06 chris Exp $ */

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

#ifndef PATHS_H

#define PATHS_H


/* Debugging */
/*#define DEBUG 1 */

/*************  Log File Locations (must be writable by **********/
/*************  the user given in the squid.conf file   **********/
#define LOG_MATCH LOGDIR"/squirm.match"
#define LOG_FAIL  LOGDIR"/squirm.fail"
#define LOG_ERROR LOGDIR"/squirm.error"
#define LOG_WHERE LOGDIR"/squirm.where"
#define LOG_DEBUG LOGDIR"/squirm.debug"
#define LOG_INFO  LOGDIR"/squirm.info"

/*************  Configuration file locations  ***********/
#define SQUIRM_CONF ETCDIR"/squirm.conf"

#endif
