/*
   Unix SMB/CIFS implementation.

   PAC Glue between Samba and the KDC

   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2005-2009
   Copyright (C) Simo Sorce <idra@samba.org> 2010

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

#include "includes.h"
#include "../libds/common/flags.h"
#include "auth/auth.h"
#include "kdc/kdc.h"
#include "param/param.h"
#include "kdc/pac-glue.h"

/* Given the right private pointer from hdb_samba4, get a PAC from the attached ldb messages */
static krb5_error_code samba_wdc_get_pac(void *priv, krb5_context context,
					 struct hdb_entry_ex *client,
					 krb5_pac *pac)
{
	TALLOC_CTX *mem_ctx;
	DATA_BLOB *pac_blob;
	krb5_error_code ret;
	NTSTATUS nt_status;

	mem_ctx = talloc_named(client->ctx, 0, "samba_get_pac context");
	if (!mem_ctx) {
		return ENOMEM;
	}

	nt_status = samba_kdc_get_pac_blob(mem_ctx, client, &pac_blob);
	if (!NT_STATUS_IS_OK(nt_status)) {
		talloc_free(mem_ctx);
		return EINVAL;
	}

	ret = samba_make_krb5_pac(context, pac_blob, pac);

	talloc_free(mem_ctx);
	return ret;
}

/* Resign (and reform, including possibly new groups) a PAC */

static krb5_error_code samba_wdc_reget_pac(void *priv, krb5_context context,
					   const krb5_principal client_principal,
					   struct hdb_entry_ex *client,
					   struct hdb_entry_ex *server, krb5_pac *pac)
{
	struct samba_kdc_entry *p = talloc_get_type(server->ctx, struct samba_kdc_entry);
	TALLOC_CTX *mem_ctx = talloc_named(p, 0, "samba_kdc_reget_pac context");
	DATA_BLOB *pac_blob;
	krb5_error_code ret;
	NTSTATUS nt_status;

	if (!mem_ctx) {
		return ENOMEM;
	}

	pac_blob = talloc_zero(mem_ctx, DATA_BLOB);
	if (!pac_blob) {
		talloc_free(mem_ctx);
		return ENOMEM;
	}

	/* The user account may be set not to want the PAC */
	if (!samba_princ_needs_pac(server)) {
		talloc_free(mem_ctx);
		return EINVAL;
	}

	nt_status = samba_kdc_update_pac_blob(mem_ctx, context,
					      pac, pac_blob);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("Building PAC failed: %s\n",
			  nt_errstr(nt_status)));
		talloc_free(mem_ctx);
		return EINVAL;
	}

	/* We now completely regenerate this pac */
	krb5_pac_free(context, *pac);

	ret = samba_make_krb5_pac(context, pac_blob, pac);

	talloc_free(mem_ctx);
	return ret;
}

static char *get_netbios_name(TALLOC_CTX *mem_ctx, HostAddresses *addrs)
{
	char *nb_name = NULL;
	size_t len;
	unsigned int i;

	for (i = 0; addrs && i < addrs->len; i++) {
		if (addrs->val[i].addr_type != KRB5_ADDRESS_NETBIOS) {
			continue;
		}
		len = MIN(addrs->val[i].address.length, 15);
		nb_name = talloc_strndup(mem_ctx,
					 addrs->val[i].address.data, len);
		if (nb_name) {
			break;
		}
	}

	if (nb_name == NULL) {
		return NULL;
	}

	/* Strip space padding */
	i = strlen(nb_name) - 1;
	while (i > 0 && nb_name[i] == ' ') {
		nb_name[i] = '\0';
	}

	return nb_name;
}

static krb5_data fill_krb5_data(void *data, size_t length)
{
	krb5_data kdata;

	kdata.data = data;
	kdata.length = length;

	return kdata;
}

static krb5_error_code samba_wdc_check_client_access(void *priv,
						     krb5_context context,
						     krb5_kdc_configuration *config,
						     hdb_entry_ex *client_ex, const char *client_name,
						     hdb_entry_ex *server_ex, const char *server_name,
						     KDC_REQ *req,
						     krb5_data *e_data)
{
	struct samba_kdc_entry *kdc_entry;
	bool password_change;
	char *workstation;
	NTSTATUS nt_status;

	kdc_entry = talloc_get_type(client_ex->ctx, struct samba_kdc_entry);
	password_change = (server_ex && server_ex->entry.flags.change_pw);
	workstation = get_netbios_name((TALLOC_CTX *)client_ex->ctx,
					req->req_body.addresses);

	nt_status = samba_kdc_check_client_access(kdc_entry,
						  client_name,
						  workstation,
						  password_change);

	if (!NT_STATUS_IS_OK(nt_status)) {
		if (NT_STATUS_EQUAL(nt_status, NT_STATUS_NO_MEMORY)) {
			return ENOMEM;
		}

		if (e_data) {
			DATA_BLOB data;

			samba_kdc_build_edata_reply(nt_status, &data);
			*e_data = fill_krb5_data(data.data, data.length);
		}

		return samba_kdc_map_policy_err(nt_status);
	}

	/* Now do the standard Heimdal check */
	return kdc_check_flags(context, config,
			       client_ex, client_name,
			       server_ex, server_name,
			       req->msg_type == krb_as_req);
}

static krb5_error_code samba_wdc_plugin_init(krb5_context context, void **ptr)
{
	*ptr = NULL;
	return 0;
}

static void samba_wdc_plugin_fini(void *ptr)
{
	return;
}

struct krb5plugin_windc_ftable windc_plugin_table = {
	.minor_version = KRB5_WINDC_PLUGING_MINOR,
	.init = samba_wdc_plugin_init,
	.fini = samba_wdc_plugin_fini,
	.pac_generate = samba_wdc_get_pac,
	.pac_verify = samba_wdc_reget_pac,
	.client_access = samba_wdc_check_client_access,
};

