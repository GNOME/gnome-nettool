/* 
 * util-mii: get basic value from a network interface
 *
 * Copyright (C) 2003 German Poo-Caaman~o <gpoo@ubiobio.cl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 */

typedef struct _mii_data_result mii_data_result;

struct _mii_data_result {
	int  has_data;
	char iface[100];
	char media[100];
	int  state;
};

mii_data_result mii_get_basic (const char *ifname);
