/* 
   Unix SMB/CIFS implementation.
   printcap parsing
   Copyright (C) Karl Auer 1993-1998

   Re-working by Martin Kiff, 1994
   
   Re-written again by Andrew Tridgell

   Modified for SVID support by Norm Jacobs, 1997

   Modified for CUPS support by Michael Sweet, 1999
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 *  Modified to call SVID/XPG4 support if printcap name is set to "lpstat"
 *  in smb.conf under Solaris.
 *
 *  Modified to call CUPS support if printcap name is set to "cups"
 *  in smb.conf.
 *
 *  Modified to call iPrint support if printcap name is set to "iprint"
 *  in smb.conf.
 */

#include "includes.h"
#include "printing/pcap.h"
#include "printer_list.h"

struct pcap_cache {
	char *name;
	char *comment;
	struct pcap_cache *next;
};

bool pcap_cache_add_specific(struct pcap_cache **ppcache, const char *name, const char *comment)
{
	struct pcap_cache *p;

	if (name == NULL || ((p = SMB_MALLOC_P(struct pcap_cache)) == NULL))
		return false;

	p->name = SMB_STRDUP(name);
	p->comment = (comment && *comment) ? SMB_STRDUP(comment) : NULL;

	DEBUG(11,("pcap_cache_add_specific: Adding name %s info %s\n",
		p->name, p->comment ? p->comment : ""));

	p->next = *ppcache;
	*ppcache = p;

	return true;
}

void pcap_cache_destroy_specific(struct pcap_cache **pp_cache)
{
	struct pcap_cache *p, *next;

	for (p = *pp_cache; p != NULL; p = next) {
		next = p->next;

		SAFE_FREE(p->name);
		SAFE_FREE(p->comment);
		SAFE_FREE(p);
	}
	*pp_cache = NULL;
}

bool pcap_cache_add(const char *name, const char *comment)
{
	NTSTATUS status;
	time_t t = time_mono(NULL);

	status = printer_list_set_printer(talloc_tos(), name, comment, t);
	return NT_STATUS_IS_OK(status);
}

bool pcap_cache_loaded(void)
{
	NTSTATUS status;
	time_t last;

	status = printer_list_get_last_refresh(&last);
	return NT_STATUS_IS_OK(status);
}

void pcap_cache_replace(const struct pcap_cache *pcache)
{
	const struct pcap_cache *p;

	for (p = pcache; p; p = p->next) {
		pcap_cache_add(p->name, p->comment);
	}
}

void pcap_cache_reload(struct tevent_context *ev,
		       struct messaging_context *msg_ctx)
{
	const char *pcap_name = lp_printcapname();
	bool pcap_reloaded = False;
	NTSTATUS status;

	DEBUG(3, ("reloading printcap cache\n"));

	/* only go looking if no printcap name supplied */
	if (pcap_name == NULL || *pcap_name == 0) {
		DEBUG(0, ("No printcap file name configured!\n"));
		return;
	}

	if (!printer_list_need_refresh()) {
		/* has been just refeshed, skip */
		DEBUG(5, ("Refresh just happend, skipping.\n"));
		return;
	}

	status = printer_list_mark_reload();
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0, ("Failed to mark printer list for reload!\n"));
		return;
	}

#ifdef HAVE_CUPS
	if (strequal(pcap_name, "cups")) {
		pcap_reloaded = cups_cache_reload(ev, msg_ctx);
		goto done;
	}
#endif

#ifdef HAVE_IPRINT
	if (strequal(pcap_name, "iprint")) {
		pcap_reloaded = iprint_cache_reload();
		goto done;
	}
#endif

#if defined(SYSV) || defined(HPUX)
	if (strequal(pcap_name, "lpstat")) {
		pcap_reloaded = sysv_cache_reload();
		goto done;
	}
#endif

#ifdef AIX
	if (strstr_m(pcap_name, "/qconfig") != NULL) {
		pcap_reloaded = aix_cache_reload();
		goto done;
	}
#endif

	pcap_reloaded = std_pcap_cache_reload(pcap_name);

done:
	DEBUG(3, ("reload status: %s\n", (pcap_reloaded) ? "ok" : "error"));

	if (pcap_reloaded) {
		/* cleanup old entries only if the operation was successful,
		 * otherwise keep around the old entries until we can
		 * successfuly reaload */
		status = printer_list_clean_old();
		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(0, ("Failed to cleanup printer list!\n"));
		}
	}

	return;
}


bool pcap_printername_ok(const char *printername)
{
	NTSTATUS status;

	status = printer_list_get_printer(talloc_tos(), printername, NULL, 0);
	return NT_STATUS_IS_OK(status);
}

/***************************************************************************
run a function on each printer name in the printcap file.
***************************************************************************/

void pcap_printer_fn_specific(const struct pcap_cache *pc,
			void (*fn)(const char *, const char *, void *),
			void *pdata)
{
	const struct pcap_cache *p;

	for (p = pc; p != NULL; p = p->next)
		fn(p->name, p->comment, pdata);

	return;
}

void pcap_printer_fn(void (*fn)(const char *, const char *, void *), void *pdata)
{
	NTSTATUS status;

	status = printer_list_run_fn(fn, pdata);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(3, ("Failed to run fn for all printers!\n"));
	}
	return;
}