/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */
/*
 * util-mii: get basic value from a network interface
 *
 * This program is based on David A. Hinds "mii-tool".
 * "mii-tool" is based on Donald Becker's "mii-diag" program.
 *
 * Copyright (C) 2000 David A. Hinds -- dhinds@pcmcia.sourceforge.org
 * Copyright (C) 2003 German Poo-Caaman~o <gpoo@ubiobio.cl>
 *
 * mii-tool is written/copyright 2000 by David A. Hinds 
 *    -- dhinds@pcmcia.sourceforge.org
 * mii-diag is written/copyright 1997-2000 by Donald Becker
 * <becker@scyld.com>
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
 * Donald Becker may be reached as becker@scyld.com, or C/O
 * Scyld Computing Corporation, 410 Severn Av., Suite 210,
 * Annapolis, MD 21403
 *
 * References
 * http://www.scyld.com/diag/mii-status.html
 * http://www.scyld.com/expert/NWay.html
 * http://www.national.com/pf/DP/DP83840.html
 */

#ifdef __linux__
#include <glib.h>
#include <glib/gprintf.h>

#include <errno.h>
#include <net/if.h>
#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif

#include <string.h>
#include <unistd.h>

#include "mii.h"
#include "util-mii.h"
#include "util-mii-wireless.h"

/*--------------------------------------------------------------------*/

static struct ifreq ifr;

static void get_legible_bits (gchar *buffer, gint bitrate);
static mii_data_result mii_get_basic_wireless (int skfd, const char *ifname);

/*--------------------------------------------------------------------*/

static int
mdio_read (int skfd, int location)
{
	struct mii_data *mii = (struct mii_data *) &ifr.ifr_data;
	mii->reg_num = location;
	if (ioctl (skfd, SIOCGMIIREG, &ifr) < 0) {
		return -1;
	}
	return mii->val_out;
}

/*--------------------------------------------------------------------*/

const struct {
	char *name;
	u_short value;
} media[] = {
	/* The order through 100baseT4 matches bits in the BMSR */
	{
	"10baseT-HD", MII_AN_10BASET_HD}, {
	"10baseT-FD", MII_AN_10BASET_FD}, {
	"100baseTx-HD", MII_AN_100BASETX_HD}, {
	"100baseTx-FD", MII_AN_100BASETX_FD}, {
	"100baseT4", MII_AN_100BASET4}, {
	"100baseTx", MII_AN_100BASETX_FD | MII_AN_100BASETX_HD}, {
"10baseT", MII_AN_10BASET_FD | MII_AN_10BASET_HD},};

static void
get_legible_bits (gchar *buffer, gint bitrate)
{
	gdouble rate = bitrate;

	if (rate >= GIGA)
		sprintf (buffer, "%g Gbps", rate / GIGA);
	else if (rate >= MEGA)
		sprintf (buffer, "%g Mbps", rate / MEGA);
	else
		sprintf (buffer, "%g kbps", rate / KILO);
}

/*--------------------------------------------------------------------*/

static char *
media_list (int mask, int best)
{
	static char buf[100];
	int i;
	*buf = '\0';
	mask >>= 5;
	for (i = 4; i >= 0; i--) {
		if (mask & (1 << i)) {
			strcat (buf, media[i].name);
			if (best)
				break;
		}
	}
	return buf;
}

/* Sample of use:

	mii_data_result data;

	data = get_basic_mii ("eth0");
	if (data.has_data == 1) {
		printf ("%s\n", data.iface);
		printf (" media: %s\n", data.media);
		printf (" state: %s\n", (data.state ? "Active" : "Inactive"));
	}
*/

/*
 * Get wireless informations & config from the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to get the settings...
 */
static mii_data_result 
mii_get_basic_wireless (int skfd, const char *ifname)
{
	struct iwreq wrq;
	mii_data_result info;

	info.has_data = 0;

	/* Set device name */
	g_strlcpy (wrq.ifr_name, ifname, IFNAMSIZ);
	
	/* Get bit rate */
	if (ioctl (skfd, SIOCGIWRATE, &wrq) >= 0) {
		info.has_data = 1;
		info.state = 1;
		g_strlcpy (info.iface, ifname, IFNAMSIZ);
		get_legible_bits (info.media, wrq.u.bitrate.value);
	}
	return info;
}

mii_data_result 
mii_get_basic (const char *ifname)
{
	int i, mii_val[32];
	int bmcr, bmsr, advert, lkpar;
	unsigned short autonegotiate = 0;
	mii_data_result data;
	struct mii_data *mii = (struct mii_data *) &ifr.ifr_data;
	int phy_id;
	int sock;

	memset (data.iface, 0, 100);
	memset (data.media, 0, 100);

	/* Open a basic socket. */
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		data.has_data = 0;
		return data;
	}
	
	/* Get the vitals from the interface. */
	strncpy (ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl (sock, SIOCGMIIPHY, &ifr) < 0) {
		if (errno != ENODEV) {
			data = mii_get_basic_wireless (sock, ifname);
			if (! data.has_data) {
				strcat (data.iface, ifname);
				strcat (data.media, "Unknown");
				data.state = 0;
			}
			return data;
		}
		data.has_data = 0;
		close (sock);
		return data;
	}
	phy_id = mii->phy_id;
	data.has_data = 1;
	
	/* Some bits in the BMSR are latched, but we can't rely on being
	   the only reader, so only the current values are meaningful */
	mdio_read (sock, MII_BMSR);
	for (i = 0; i < 8; i++)
		mii_val[i] = mdio_read (sock, i);

    if (mii_val[MII_BMCR] == 0xffff) {
	    data.has_data = 0;
		close (sock);
    	return data;
    }

	/* Descriptive rename. */
	bmcr = mii_val[MII_BMCR];
	bmsr = mii_val[MII_BMSR];
	advert = mii_val[MII_ANAR];
	lkpar = mii_val[MII_ANLPAR];

	sprintf (data.iface, "%s", ifr.ifr_name);
	if (bmcr & MII_BMCR_AN_ENA) {
		if (bmsr & MII_BMSR_AN_COMPLETE) {
			if (advert & lkpar) {
				strcat (data.media, media_list (advert & lkpar, 1));
				autonegotiate = 1;
			}
		}
	}
	if (! autonegotiate) {
		sprintf (data.media, "%s Mbit %sD",
			 (bmcr & MII_BMCR_100MBIT) ? "100" : "10",
			 (bmcr & MII_BMCR_DUPLEX) ? "F" : "H");
	}
	data.state = (bmsr & MII_BMSR_LINK_VALID);

	close (sock);
	return data;
}
#endif
