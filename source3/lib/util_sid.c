/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 		1992-1998
   Copyright (C) Luke Kenneth Caseson Leighton 	1998-1999
   Copyright (C) Jeremy Allison  		1999
   Copyright (C) Stefan (metze) Metzmacher 	2002
   Copyright (C) Simo Sorce 			2002
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2005

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
#include "../librpc/gen_ndr/ndr_security.h"
#include "../librpc/gen_ndr/netlogon.h"
#include "../libcli/security/dom_sid.h"

/*
 * Some useful sids, more well known sids can be found at
 * http://support.microsoft.com/kb/243330/EN-US/
 */


const struct dom_sid global_sid_World_Domain =               /* Everyone domain */
{ 1, 0, {0,0,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_World =                      /* Everyone */
{ 1, 1, {0,0,0,0,0,1}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Creator_Owner_Domain =       /* Creator Owner domain */
{ 1, 0, {0,0,0,0,0,3}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_NT_Authority =    		/* NT Authority */
{ 1, 0, {0,0,0,0,0,5}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_System =			/* System */
{ 1, 1, {0,0,0,0,0,5}, {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_NULL =            		/* NULL sid */
{ 1, 1, {0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Authenticated_Users =	/* All authenticated rids */
{ 1, 1, {0,0,0,0,0,5}, {11,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
#if 0
/* for documentation */
const struct dom_sid global_sid_Restriced =			/* Restriced Code */
{ 1, 1, {0,0,0,0,0,5}, {12,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
#endif
const struct dom_sid global_sid_Network =			/* Network rids */
{ 1, 1, {0,0,0,0,0,5}, {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const struct dom_sid global_sid_Creator_Owner =		/* Creator Owner */
{ 1, 1, {0,0,0,0,0,3}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Creator_Group =		/* Creator Group */
{ 1, 1, {0,0,0,0,0,3}, {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Anonymous =			/* Anonymous login */
{ 1, 1, {0,0,0,0,0,5}, {7,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const struct dom_sid global_sid_Builtin = 			/* Local well-known domain */
{ 1, 1, {0,0,0,0,0,5}, {32,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Administrators =	/* Builtin administrators */
{ 1, 2, {0,0,0,0,0,5}, {32,544,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Users =		/* Builtin users */
{ 1, 2, {0,0,0,0,0,5}, {32,545,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Guests =		/* Builtin guest users */
{ 1, 2, {0,0,0,0,0,5}, {32,546,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Power_Users =	/* Builtin power users */
{ 1, 2, {0,0,0,0,0,5}, {32,547,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Account_Operators =	/* Builtin account operators */
{ 1, 2, {0,0,0,0,0,5}, {32,548,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Server_Operators =	/* Builtin server operators */
{ 1, 2, {0,0,0,0,0,5}, {32,549,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Print_Operators =	/* Builtin print operators */
{ 1, 2, {0,0,0,0,0,5}, {32,550,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Backup_Operators =	/* Builtin backup operators */
{ 1, 2, {0,0,0,0,0,5}, {32,551,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_Replicator =		/* Builtin replicator */
{ 1, 2, {0,0,0,0,0,5}, {32,552,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Builtin_PreWin2kAccess =	/* Builtin pre win2k access */
{ 1, 2, {0,0,0,0,0,5}, {32,554,0,0,0,0,0,0,0,0,0,0,0,0,0}};

const struct dom_sid global_sid_Unix_Users =			/* Unmapped Unix users */
{ 1, 1, {0,0,0,0,0,22}, {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
const struct dom_sid global_sid_Unix_Groups =			/* Unmapped Unix groups */
{ 1, 1, {0,0,0,0,0,22}, {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

/* Unused, left here for documentary purposes */
#if 0
#define SECURITY_NULL_SID_AUTHORITY    0
#define SECURITY_WORLD_SID_AUTHORITY   1
#define SECURITY_LOCAL_SID_AUTHORITY   2
#define SECURITY_CREATOR_SID_AUTHORITY 3
#define SECURITY_NT_AUTHORITY          5
#endif

static struct dom_sid system_sid_array[1] =
{ { 1, 1, {0,0,0,0,0,5}, {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0}} };
static const struct security_token system_token = {
	.num_sids       = ARRAY_SIZE(system_sid_array),
	.sids           = system_sid_array,
	.privilege_mask = SE_ALL_PRIVS
};

/****************************************************************************
 Lookup string names for SID types.
****************************************************************************/

static const struct {
	enum lsa_SidType sid_type;
	const char *string;
} sid_name_type[] = {
	{SID_NAME_USER, "User"},
	{SID_NAME_DOM_GRP, "Domain Group"},
	{SID_NAME_DOMAIN, "Domain"},
	{SID_NAME_ALIAS, "Local Group"},
	{SID_NAME_WKN_GRP, "Well-known Group"},
	{SID_NAME_DELETED, "Deleted Account"},
	{SID_NAME_INVALID, "Invalid Account"},
	{SID_NAME_UNKNOWN, "UNKNOWN"},
	{SID_NAME_COMPUTER, "Computer"},

 	{(enum lsa_SidType)0, NULL}
};

const char *sid_type_lookup(uint32 sid_type) 
{
	int i = 0;

	/* Look through list */
	while(sid_name_type[i].sid_type != 0) {
		if (sid_name_type[i].sid_type == sid_type)
			return sid_name_type[i].string;
		i++;
	}

	/* Default return */
	return "SID *TYPE* is INVALID";
}

/**************************************************************************
 Create the SYSTEM token.
***************************************************************************/

const struct security_token *get_system_token(void)
{
	return &system_token;
}

/*****************************************************************
 Convert a SID to an ascii string.
*****************************************************************/

char *sid_to_fstring(fstring sidstr_out, const struct dom_sid *sid)
{
	char *str = sid_string_talloc(talloc_tos(), sid);
	fstrcpy(sidstr_out, str);
	TALLOC_FREE(str);
	return sidstr_out;
}

/*****************************************************************
 Essentially a renamed dom_sid_string from
 ../libcli/security/dom_sid.c with a panic if it didn't work.
*****************************************************************/

char *sid_string_talloc(TALLOC_CTX *mem_ctx, const struct dom_sid *sid)
{
	char *result = dom_sid_string(mem_ctx, sid);
	SMB_ASSERT(result != NULL);
	return result;
}

/*****************************************************************
 Useful function for debug lines.
*****************************************************************/

char *sid_string_dbg(const struct dom_sid *sid)
{
	return sid_string_talloc(talloc_tos(), sid);
}

/*****************************************************************
 Use with care!
*****************************************************************/

char *sid_string_tos(const struct dom_sid *sid)
{
	return sid_string_talloc(talloc_tos(), sid);
}

bool sid_compose(struct dom_sid *dst, const struct dom_sid *domain_sid, uint32 rid)
{
	sid_copy(dst, domain_sid);
	return sid_append_rid(dst, rid);
}

/*****************************************************************
 Removes the last rid from the end of a sid
*****************************************************************/  

bool sid_split_rid(struct dom_sid *sid, uint32 *rid)
{
	if (sid->num_auths > 0) {
		sid->num_auths--;
		*rid = sid->sub_auths[sid->num_auths];
		return True;
	}
	return False;
}

/*****************************************************************
 Return the last rid from the end of a sid
*****************************************************************/  

bool sid_peek_rid(const struct dom_sid *sid, uint32 *rid)
{
	if (!sid || !rid)
		return False;		

	if (sid->num_auths > 0) {
		*rid = sid->sub_auths[sid->num_auths - 1];
		return True;
	}
	return False;
}

/*****************************************************************
 Return the last rid from the end of a sid
 and check the sid against the exp_dom_sid  
*****************************************************************/  

bool sid_peek_check_rid(const struct dom_sid *exp_dom_sid, const struct dom_sid *sid, uint32 *rid)
{
	if (!exp_dom_sid || !sid || !rid)
		return False;

	if (sid->num_auths != (exp_dom_sid->num_auths+1)) {
		return False;
	}

	if (dom_sid_compare_domain(exp_dom_sid, sid)!=0){
		*rid=(-1);
		return False;
	}

	return sid_peek_rid(sid, rid);
}

/*****************************************************************
 Copies a sid
*****************************************************************/  

void sid_copy(struct dom_sid *dst, const struct dom_sid *src)
{
	int i;

	ZERO_STRUCTP(dst);

	dst->sid_rev_num = src->sid_rev_num;
	dst->num_auths = src->num_auths;

	memcpy(&dst->id_auth[0], &src->id_auth[0], sizeof(src->id_auth));

	for (i = 0; i < src->num_auths; i++)
		dst->sub_auths[i] = src->sub_auths[i];
}

/*****************************************************************
 Write a sid out into on-the-wire format.
*****************************************************************/  

bool sid_linearize(char *outbuf, size_t len, const struct dom_sid *sid)
{
	size_t i;

	if (len < ndr_size_dom_sid(sid, 0))
		return False;

	SCVAL(outbuf,0,sid->sid_rev_num);
	SCVAL(outbuf,1,sid->num_auths);
	memcpy(&outbuf[2], sid->id_auth, 6);
	for(i = 0; i < sid->num_auths; i++)
		SIVAL(outbuf, 8 + (i*4), sid->sub_auths[i]);

	return True;
}

/*****************************************************************
 Parse a on-the-wire SID to a struct dom_sid.
*****************************************************************/  

bool sid_parse(const char *inbuf, size_t len, struct dom_sid *sid)
{
	enum ndr_err_code ndr_err;
	DATA_BLOB in = data_blob_const(inbuf, len);
	ndr_err = ndr_pull_struct_blob_all(&in, NULL, sid,
					   (ndr_pull_flags_fn_t)ndr_pull_dom_sid);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		return false;
	}
	return true;
}

/*****************************************************************
 Returns true if SID is internal (and non-mappable).
*****************************************************************/

bool non_mappable_sid(struct dom_sid *sid)
{
	struct dom_sid dom;
	uint32 rid;

	sid_copy(&dom, sid);
	sid_split_rid(&dom, &rid);

	if (dom_sid_equal(&dom, &global_sid_Builtin))
		return True;

	if (dom_sid_equal(&dom, &global_sid_NT_Authority))
		return True;

	return False;
}

/*****************************************************************
 Return the binary string representation of a struct dom_sid.
 Caller must free.
*****************************************************************/

char *sid_binstring_hex(const struct dom_sid *sid)
{
	char *buf, *s;
	int len = ndr_size_dom_sid(sid, 0);
	buf = (char *)SMB_MALLOC(len);
	if (!buf)
		return NULL;
	sid_linearize(buf, len, sid);
	s = binary_string(buf, len);
	free(buf);
	return s;
}

/********************************************************************
 Add SID to an array SIDs
********************************************************************/

NTSTATUS add_sid_to_array(TALLOC_CTX *mem_ctx, const struct dom_sid *sid,
			  struct dom_sid **sids, uint32_t *num)
{
	*sids = TALLOC_REALLOC_ARRAY(mem_ctx, *sids, struct dom_sid,
					     (*num)+1);
	if (*sids == NULL) {
		*num = 0;
		return NT_STATUS_NO_MEMORY;
	}

	sid_copy(&((*sids)[*num]), sid);
	*num += 1;

	return NT_STATUS_OK;
}


/********************************************************************
 Add SID to an array SIDs ensuring that it is not already there
********************************************************************/

NTSTATUS add_sid_to_array_unique(TALLOC_CTX *mem_ctx, const struct dom_sid *sid,
				 struct dom_sid **sids, uint32_t *num_sids)
{
	size_t i;

	for (i=0; i<(*num_sids); i++) {
		if (dom_sid_compare(sid, &(*sids)[i]) == 0)
			return NT_STATUS_OK;
	}

	return add_sid_to_array(mem_ctx, sid, sids, num_sids);
}

/********************************************************************
 Remove SID from an array
********************************************************************/

void del_sid_from_array(const struct dom_sid *sid, struct dom_sid **sids, size_t *num)
{
	struct dom_sid *sid_list = *sids;
	size_t i;

	for ( i=0; i<*num; i++ ) {

		/* if we find the SID, then decrement the count
		   and break out of the loop */

		if ( dom_sid_equal(sid, &sid_list[i]) ) {
			*num -= 1;
			break;
		}
	}

	/* This loop will copy the remainder of the array 
	   if i < num of sids ni the array */

	for ( ; i<*num; i++ ) 
		sid_copy( &sid_list[i], &sid_list[i+1] );

	return;
}

bool add_rid_to_array_unique(TALLOC_CTX *mem_ctx,
				    uint32 rid, uint32 **pp_rids, size_t *p_num)
{
	size_t i;

	for (i=0; i<*p_num; i++) {
		if ((*pp_rids)[i] == rid)
			return True;
	}

	*pp_rids = TALLOC_REALLOC_ARRAY(mem_ctx, *pp_rids, uint32, *p_num+1);

	if (*pp_rids == NULL) {
		*p_num = 0;
		return False;
	}

	(*pp_rids)[*p_num] = rid;
	*p_num += 1;
	return True;
}

bool is_null_sid(const struct dom_sid *sid)
{
	static const struct dom_sid null_sid = {0};
	return dom_sid_equal(sid, &null_sid);
}

bool is_sid_in_token(const struct security_token *token, const struct dom_sid *sid)
{
        int i;

        for (i=0; i<token->num_sids; i++) {
                if (dom_sid_compare(sid, &token->sids[i]) == 0)
                        return true;
        }
        return false;
}

NTSTATUS sid_array_from_info3(TALLOC_CTX *mem_ctx,
			      const struct netr_SamInfo3 *info3,
			      struct dom_sid **user_sids,
			      uint32_t *num_user_sids,
			      bool include_user_group_rid,
			      bool skip_ressource_groups)
{
	NTSTATUS status;
	struct dom_sid sid;
	struct dom_sid *sid_array = NULL;
	uint32_t num_sids = 0;
	int i;

	if (include_user_group_rid) {
		if (!sid_compose(&sid, info3->base.domain_sid, info3->base.rid)) {
			DEBUG(3, ("could not compose user SID from rid 0x%x\n",
				  info3->base.rid));
			return NT_STATUS_INVALID_PARAMETER;
		}
		status = add_sid_to_array(mem_ctx, &sid, &sid_array, &num_sids);
		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(3, ("could not append user SID from rid 0x%x\n",
				  info3->base.rid));
			return status;
		}
	}

	if (!sid_compose(&sid, info3->base.domain_sid, info3->base.primary_gid)) {
		DEBUG(3, ("could not compose group SID from rid 0x%x\n",
			  info3->base.primary_gid));
		return NT_STATUS_INVALID_PARAMETER;
	}
	status = add_sid_to_array(mem_ctx, &sid, &sid_array, &num_sids);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(3, ("could not append group SID from rid 0x%x\n",
			  info3->base.rid));
		return status;
	}

	for (i = 0; i < info3->base.groups.count; i++) {
		/* Don't add the primary group sid twice. */
		if (info3->base.primary_gid == info3->base.groups.rids[i].rid) {
			continue;
		}
		if (!sid_compose(&sid, info3->base.domain_sid,
				 info3->base.groups.rids[i].rid)) {
			DEBUG(3, ("could not compose SID from additional group "
				  "rid 0x%x\n", info3->base.groups.rids[i].rid));
			return NT_STATUS_INVALID_PARAMETER;
		}
		status = add_sid_to_array(mem_ctx, &sid, &sid_array, &num_sids);
		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(3, ("could not append SID from additional group "
				  "rid 0x%x\n", info3->base.groups.rids[i].rid));
			return status;
		}
	}

	/* Copy 'other' sids.  We need to do sid filtering here to
 	   prevent possible elevation of privileges.  See:

           http://www.microsoft.com/windows2000/techinfo/administration/security/sidfilter.asp
         */

	for (i = 0; i < info3->sidcount; i++) {

		if (skip_ressource_groups &&
		    (info3->sids[i].attributes & SE_GROUP_RESOURCE)) {
			continue;
		}

		status = add_sid_to_array(mem_ctx, info3->sids[i].sid,
				      &sid_array, &num_sids);
		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(3, ("could not add SID to array: %s\n",
				  sid_string_dbg(info3->sids[i].sid)));
			return status;
		}
	}

	*user_sids = sid_array;
	*num_user_sids = num_sids;

	return NT_STATUS_OK;
}