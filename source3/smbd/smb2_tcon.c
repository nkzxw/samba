/*
   Unix SMB/CIFS implementation.
   Core SMB2 server

   Copyright (C) Stefan Metzmacher 2009

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
#include "smbd/globals.h"
#include "../libcli/smb/smb_common.h"

static NTSTATUS smbd_smb2_tree_connect(struct smbd_smb2_request *req,
				       const char *in_path,
				       uint8_t *out_share_type,
				       uint32_t *out_share_flags,
				       uint32_t *out_capabilities,
				       uint32_t *out_maximal_access,
				       uint32_t *out_tree_id);

NTSTATUS smbd_smb2_request_process_tcon(struct smbd_smb2_request *req)
{
	const uint8_t *inbody;
	int i = req->current_idx;
	uint8_t *outhdr;
	DATA_BLOB outbody;
	size_t expected_body_size = 0x09;
	size_t body_size;
	uint16_t in_path_offset;
	uint16_t in_path_length;
	DATA_BLOB in_path_buffer;
	char *in_path_string;
	size_t in_path_string_size;
	uint8_t out_share_type = 0;
	uint32_t out_share_flags = 0;
	uint32_t out_capabilities = 0;
	uint32_t out_maximal_access = 0;
	uint32_t out_tree_id = 0;
	NTSTATUS status;
	bool ok;

	if (req->in.vector[i+1].iov_len != (expected_body_size & 0xFFFFFFFE)) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	inbody = (const uint8_t *)req->in.vector[i+1].iov_base;

	body_size = SVAL(inbody, 0x00);
	if (body_size != expected_body_size) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	in_path_offset = SVAL(inbody, 0x04);
	in_path_length = SVAL(inbody, 0x06);

	if (in_path_offset != (SMB2_HDR_BODY + (body_size & 0xFFFFFFFE))) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	if (in_path_length > req->in.vector[i+2].iov_len) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	in_path_buffer.data = (uint8_t *)req->in.vector[i+2].iov_base;
	in_path_buffer.length = in_path_length;

	ok = convert_string_talloc(req, CH_UTF16, CH_UNIX,
				   in_path_buffer.data,
				   in_path_buffer.length,
				   &in_path_string,
				   &in_path_string_size, false);
	if (!ok) {
		return smbd_smb2_request_error(req, NT_STATUS_ILLEGAL_CHARACTER);
	}

	status = smbd_smb2_tree_connect(req, in_path_string,
					&out_share_type,
					&out_share_flags,
					&out_capabilities,
					&out_maximal_access,
					&out_tree_id);
	if (!NT_STATUS_IS_OK(status)) {
		return smbd_smb2_request_error(req, status);
	}

	outhdr = (uint8_t *)req->out.vector[i].iov_base;

	outbody = data_blob_talloc(req->out.vector, NULL, 0x10);
	if (outbody.data == NULL) {
		return smbd_smb2_request_error(req, NT_STATUS_NO_MEMORY);
	}

	SIVAL(outhdr, SMB2_HDR_TID, out_tree_id);

	SSVAL(outbody.data, 0x00, 0x10);	/* struct size */
	SCVAL(outbody.data, 0x02,
	      out_share_type);			/* share type */
	SCVAL(outbody.data, 0x03, 0);		/* reserved */
	SIVAL(outbody.data, 0x04,
	      out_share_flags);			/* share flags */
	SIVAL(outbody.data, 0x08,
	      out_capabilities);		/* capabilities */
	SIVAL(outbody.data, 0x0C,
	      out_maximal_access);		/* maximal access */

	return smbd_smb2_request_done(req, outbody, NULL);
}

static int smbd_smb2_tcon_destructor(struct smbd_smb2_tcon *tcon)
{
	if (tcon->session == NULL) {
		return 0;
	}

	idr_remove(tcon->session->tcons.idtree, tcon->tid);
	DLIST_REMOVE(tcon->session->tcons.list, tcon);

	if (tcon->compat_conn) {
		set_current_service(tcon->compat_conn, 0, true);
		close_cnum(tcon->compat_conn, tcon->session->vuid);
	}

	tcon->compat_conn = NULL;
	tcon->tid = 0;
	tcon->session = NULL;

	return 0;
}

static NTSTATUS smbd_smb2_tree_connect(struct smbd_smb2_request *req,
				       const char *in_path,
				       uint8_t *out_share_type,
				       uint32_t *out_share_flags,
				       uint32_t *out_capabilities,
				       uint32_t *out_maximal_access,
				       uint32_t *out_tree_id)
{
	const char *share = in_path;
	fstring service;
	int snum = -1;
	struct smbd_smb2_tcon *tcon;
	connection_struct *compat_conn = NULL;
	user_struct *compat_vuser = req->session->compat_vuser;
	int id;
	NTSTATUS status;

	if (strncmp(share, "\\\\", 2) == 0) {
		const char *p = strchr(share+2, '\\');
		if (p) {
			share = p + 1;
		}
	}

	DEBUG(10,("smbd_smb2_tree_connect: path[%s] share[%s]\n",
		  in_path, share));

	fstrcpy(service, share);

	strlower_m(service);

	/* TODO: do more things... */
	if (strequal(service,HOMES_NAME)) {
		if (compat_vuser->homes_snum == -1) {
			DEBUG(2, ("[homes] share not available for "
				"user %s because it was not found "
				"or created at session setup "
				"time\n",
				compat_vuser->server_info->unix_name));
			return NT_STATUS_BAD_NETWORK_NAME;
		}
		snum = compat_vuser->homes_snum;
	} else if ((compat_vuser->homes_snum != -1)
                   && strequal(service,
			lp_servicename(compat_vuser->homes_snum))) {
		snum = compat_vuser->homes_snum;
	} else {
		snum = find_service(service);
	}

	if (snum < 0) {
		DEBUG(3,("smbd_smb2_tree_connect: couldn't find service %s\n",
			 service));
		return NT_STATUS_BAD_NETWORK_NAME;
	}

	/* create a new tcon as child of the session */
	tcon = talloc_zero(req->session, struct smbd_smb2_tcon);
	if (tcon == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	id = idr_get_new_random(req->session->tcons.idtree,
				tcon,
				req->session->tcons.limit);
	if (id == -1) {
		TALLOC_FREE(tcon);
		return NT_STATUS_INSUFFICIENT_RESOURCES;
	}
	tcon->tid = id;
	tcon->snum = snum;

	DLIST_ADD_END(req->session->tcons.list, tcon,
		      struct smbd_smb2_tcon *);
	tcon->session = req->session;
	talloc_set_destructor(tcon, smbd_smb2_tcon_destructor);

	compat_conn = make_connection_snum(req->sconn,
					snum, req->session->compat_vuser,
					data_blob_null, "???",
					&status);
	if (compat_conn == NULL) {
		TALLOC_FREE(tcon);
		return status;
	}
	tcon->compat_conn = talloc_move(tcon, &compat_conn);
	tcon->compat_conn->cnum = tcon->tid;

	if (IS_PRINT(tcon->compat_conn)) {
		*out_share_type = SMB2_SHARE_TYPE_PRINT;
	} else if (IS_IPC(tcon->compat_conn)) {
		*out_share_type = SMB2_SHARE_TYPE_PIPE;
	} else {
		*out_share_type = SMB2_SHARE_TYPE_DISK;
	}

	*out_share_flags = SMB2_SHAREFLAG_ALLOW_NAMESPACE_CACHING;

	if (lp_msdfs_root(SNUM(tcon->compat_conn)) && lp_host_msdfs()) {
		*out_share_flags |= (SMB2_SHAREFLAG_DFS|SMB2_SHAREFLAG_DFS_ROOT);
		*out_capabilities = SMB2_SHARE_CAP_DFS;
	} else {
		*out_capabilities = 0;
	}

	switch(lp_csc_policy(SNUM(tcon->compat_conn))) {
	case CSC_POLICY_MANUAL:
		break;
	case CSC_POLICY_DOCUMENTS:
		*out_share_flags |= SMB2_SHAREFLAG_AUTO_CACHING;
		break;
	case CSC_POLICY_PROGRAMS:
		*out_share_flags |= SMB2_SHAREFLAG_VDO_CACHING;
		break;
	case CSC_POLICY_DISABLE:
		*out_share_flags |= SMB2_SHAREFLAG_NO_CACHING;
		break;
	default:
		break;
	}

	*out_maximal_access = FILE_GENERIC_ALL;

	*out_tree_id = tcon->tid;
	return NT_STATUS_OK;
}

NTSTATUS smbd_smb2_request_check_tcon(struct smbd_smb2_request *req)
{
	const uint8_t *inhdr;
	const uint8_t *outhdr;
	int i = req->current_idx;
	uint32_t in_tid;
	void *p;
	struct smbd_smb2_tcon *tcon;
	bool chained_fixup = false;

	inhdr = (const uint8_t *)req->in.vector[i+0].iov_base;

	in_tid = IVAL(inhdr, SMB2_HDR_TID);

	if (in_tid == (0xFFFFFFFF)) {
		if (req->async) {
			/*
			 * async request - fill in tid from
			 * already setup out.vector[].iov_base.
			 */
			outhdr = (const uint8_t *)req->out.vector[i].iov_base;
			in_tid = IVAL(outhdr, SMB2_HDR_TID);
		} else if (i > 2) {
			/*
			 * Chained request - fill in tid from
			 * the previous request out.vector[].iov_base.
			 */
			outhdr = (const uint8_t *)req->out.vector[i-3].iov_base;
			in_tid = IVAL(outhdr, SMB2_HDR_TID);
			chained_fixup = true;
		}
	}

	/* lookup an existing session */
	p = idr_find(req->session->tcons.idtree, in_tid);
	if (p == NULL) {
		return NT_STATUS_NETWORK_NAME_DELETED;
	}
	tcon = talloc_get_type_abort(p, struct smbd_smb2_tcon);

	if (!change_to_user(tcon->compat_conn,req->session->vuid)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	/* should we pass FLAG_CASELESS_PATHNAMES here? */
	if (!set_current_service(tcon->compat_conn, 0, true)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	req->tcon = tcon;

	if (chained_fixup) {
		/* Fix up our own outhdr. */
		outhdr = (const uint8_t *)req->out.vector[i].iov_base;
		SIVAL(outhdr, SMB2_HDR_TID, in_tid);
	}

	return NT_STATUS_OK;
}

NTSTATUS smbd_smb2_request_process_tdis(struct smbd_smb2_request *req)
{
	const uint8_t *inbody;
	int i = req->current_idx;
	DATA_BLOB outbody;
	size_t expected_body_size = 0x04;
	size_t body_size;

	if (req->in.vector[i+1].iov_len != (expected_body_size & 0xFFFFFFFE)) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	inbody = (const uint8_t *)req->in.vector[i+1].iov_base;

	body_size = SVAL(inbody, 0x00);
	if (body_size != expected_body_size) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	/*
	 * TODO: cancel all outstanding requests on the tcon
	 *       and delete all file handles.
	 */
	TALLOC_FREE(req->tcon);

	outbody = data_blob_talloc(req->out.vector, NULL, 0x04);
	if (outbody.data == NULL) {
		return smbd_smb2_request_error(req, NT_STATUS_NO_MEMORY);
	}

	SSVAL(outbody.data, 0x00, 0x04);	/* struct size */
	SSVAL(outbody.data, 0x02, 0);		/* reserved */

	return smbd_smb2_request_done(req, outbody, NULL);
}