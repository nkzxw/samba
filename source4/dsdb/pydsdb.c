/* 
   Unix SMB/CIFS implementation.
   Copyright (C) Jelmer Vernooij <jelmer@samba.org> 2007-2010
   Copyright (C) Matthias Dieter Wallnöfer          2009
   
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

#include <Python.h>
#include "includes.h"
#include "libcli/util/pyerrors.h"
#include "dsdb/samdb/samdb.h"
#include "lib/ldb/pyldb.h"
#include "libcli/security/security.h"
#include "librpc/ndr/libndr.h"
#include "system/kerberos.h"
#include "auth/kerberos/kerberos.h"
/* FIXME: These should be in a header file somewhere, once we finish moving
 * away from SWIG .. */
#define PyErr_LDB_OR_RAISE(py_ldb, ldb) \
/*	if (!PyLdb_Check(py_ldb)) { \
		PyErr_SetString(py_ldb_get_exception(), "Ldb connection object required"); \
		return NULL; \
	} */\
	ldb = PyLdb_AsLdbContext(py_ldb);

static PyObject *py_ldb_get_exception(void)
{
	PyObject *mod = PyImport_ImportModule("ldb");
	if (mod == NULL)
		return NULL;

	return PyObject_GetAttrString(mod, "LdbError");
}

static void PyErr_SetLdbError(PyObject *error, int ret, struct ldb_context *ldb_ctx)
{
	if (ret == LDB_ERR_PYTHON_EXCEPTION)
		return; /* Python exception should already be set, just keep that */

	PyErr_SetObject(error, 
			Py_BuildValue(discard_const_p(char, "(i,s)"), ret,
			ldb_ctx == NULL?ldb_strerror(ret):ldb_errstring(ldb_ctx)));
}

static PyObject *py_samdb_server_site_name(PyObject *self, PyObject *args)
{
	PyObject *py_ldb, *result;
	struct ldb_context *ldb;
	const char *site;
	TALLOC_CTX *mem_ctx;

	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	mem_ctx = talloc_new(NULL);

	site = samdb_server_site_name(ldb, mem_ctx);
	if (site == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to find server site");
		talloc_free(mem_ctx);
		return NULL;
	}

	result = PyString_FromString(site);
	talloc_free(mem_ctx);
	return result;
}

static PyObject *py_dsdb_convert_schema_to_openldap(PyObject *self,
													PyObject *args)
{
	char *target_str, *mapping;
	PyObject *py_ldb;
	struct ldb_context *ldb;
	PyObject *ret;
	char *retstr;

	if (!PyArg_ParseTuple(args, "Oss", &py_ldb, &target_str, &mapping))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	retstr = dsdb_convert_schema_to_openldap(ldb, target_str, mapping);
	if (retstr == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
						"dsdb_convert_schema_to_openldap failed");
		return NULL;
	} 

	ret = PyString_FromString(retstr);
	talloc_free(retstr);
	return ret;
}

static PyObject *py_samdb_set_domain_sid(PyLdbObject *self, PyObject *args)
{ 
	PyObject *py_ldb, *py_sid;
	struct ldb_context *ldb;
	struct dom_sid *sid;
	bool ret;

	if (!PyArg_ParseTuple(args, "OO", &py_ldb, &py_sid))
		return NULL;
	
	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	sid = dom_sid_parse_talloc(NULL, PyString_AsString(py_sid));

	ret = samdb_set_domain_sid(ldb, sid);
	if (!ret) {
		PyErr_SetString(PyExc_RuntimeError, "set_domain_sid failed");
		return NULL;
	} 
	Py_RETURN_NONE;
}

static PyObject *py_samdb_set_ntds_settings_dn(PyLdbObject *self, PyObject *args)
{ 
	PyObject *py_ldb, *py_ntds_settings_dn;
	struct ldb_context *ldb;
	struct ldb_dn *ntds_settings_dn;
	TALLOC_CTX *tmp_ctx;
	bool ret;

	if (!PyArg_ParseTuple(args, "OO", &py_ldb, &py_ntds_settings_dn))
		return NULL;
	
	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	tmp_ctx = talloc_new(NULL);
	if (tmp_ctx == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	if (!PyObject_AsDn(tmp_ctx, py_ntds_settings_dn, ldb, &ntds_settings_dn)) {
		return NULL;
	}

	ret = samdb_set_ntds_settings_dn(ldb, ntds_settings_dn);
	talloc_free(tmp_ctx);
	if (!ret) {
		PyErr_SetString(PyExc_RuntimeError, "set_ntds_settings_dn failed");
		return NULL;
	} 
	Py_RETURN_NONE;
}

static PyObject *py_samdb_get_domain_sid(PyLdbObject *self, PyObject *args)
{ 
	PyObject *py_ldb;
	struct ldb_context *ldb;
	const struct dom_sid *sid;
	PyObject *ret;
	char *retstr;

	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;
	
	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	sid = samdb_domain_sid(ldb);
	if (!sid) {
		PyErr_SetString(PyExc_RuntimeError, "samdb_domain_sid failed");
		return NULL;
	} 
	retstr = dom_sid_string(NULL, sid);
	ret = PyString_FromString(retstr);
	talloc_free(retstr);
	return ret;
}

static PyObject *py_samdb_ntds_invocation_id(PyObject *self, PyObject *args)
{
	PyObject *py_ldb, *result;
	struct ldb_context *ldb;
	TALLOC_CTX *mem_ctx;
	const struct GUID *guid;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "O", &py_ldb)) {
		talloc_free(mem_ctx);
		return NULL;
	}

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	guid = samdb_ntds_invocation_id(ldb);
	if (guid == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
						"Failed to find NTDS invocation ID");
		talloc_free(mem_ctx);
		return NULL;
	}

	result = PyString_FromString(GUID_string(mem_ctx, guid));
	talloc_free(mem_ctx);
	return result;
}

static PyObject *py_dsdb_get_oid_from_attid(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	struct ldb_context *ldb;
	uint32_t attid;
	struct dsdb_schema *schema;
	const char *oid;
	PyObject *ret;
	TALLOC_CTX *mem_ctx;
	WERROR status;

	if (!PyArg_ParseTuple(args, "Oi", &py_ldb, &attid))
		return NULL;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
	   PyErr_NoMemory();
	   return NULL;
	}

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	schema = dsdb_get_schema(ldb, NULL);

	if (!schema) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to find a schema from ldb \n");
		talloc_free(mem_ctx);
		return NULL;
	}
	
	status = dsdb_schema_pfm_oid_from_attid(schema->prefixmap, attid,
	                                        mem_ctx, &oid);
	PyErr_WERROR_IS_ERR_RAISE(status);

	ret = PyString_FromString(oid);

	talloc_free(mem_ctx);

	return ret;
}


static PyObject *py_dsdb_get_attid_from_lDAPDisplayName(PyObject *self, PyObject *args)
{
	PyObject *py_ldb, *is_schema_nc;
	struct ldb_context *ldb;
	struct dsdb_schema *schema;
	const char *ldap_display_name;
	bool schema_nc = false;
	const struct dsdb_attribute *a;
	uint32_t attid;

	if (!PyArg_ParseTuple(args, "OsO", &py_ldb, &ldap_display_name, &is_schema_nc))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	if (is_schema_nc) {
		if (!PyBool_Check(is_schema_nc)) {
			PyErr_SetString(PyExc_TypeError, "Expected boolean is_schema_nc");
			return NULL;
		}
		if (is_schema_nc == Py_True) {
			schema_nc = true;
		}
	}

	schema = dsdb_get_schema(ldb, NULL);

	if (!schema) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to find a schema from ldb");
		return NULL;
	}

	a = dsdb_attribute_by_lDAPDisplayName(schema, ldap_display_name);
	if (a == NULL) {
		PyErr_Format(PyExc_RuntimeError, "Failed to find attribute '%s'", ldap_display_name);
		return NULL;
	}

	attid = dsdb_attribute_get_attid(a, schema_nc);

	return PyLong_FromUnsignedLong(attid);
}

static PyObject *py_dsdb_set_ntds_invocation_id(PyObject *self, PyObject *args)
{
	PyObject *py_ldb, *py_guid;
	bool ret;
	struct GUID guid;
	struct ldb_context *ldb;
	if (!PyArg_ParseTuple(args, "OO", &py_ldb, &py_guid))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);
	GUID_from_string(PyString_AsString(py_guid), &guid);

	ret = samdb_set_ntds_invocation_id(ldb, &guid);
	if (!ret) {
		PyErr_SetString(PyExc_RuntimeError, "set_ntds_invocation_id failed");
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *py_samdb_ntds_objectGUID(PyObject *self, PyObject *args)
{
	PyObject *py_ldb, *result;
	struct ldb_context *ldb;
	TALLOC_CTX *mem_ctx;
	const struct GUID *guid;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "O", &py_ldb)) {
		talloc_free(mem_ctx);
		return NULL;
	}

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	guid = samdb_ntds_objectGUID(ldb);
	if (guid == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to find NTDS GUID");
		talloc_free(mem_ctx);
		return NULL;
	}

	result = PyString_FromString(GUID_string(mem_ctx, guid));
	talloc_free(mem_ctx);
	return result;
}

static PyObject *py_dsdb_set_global_schema(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	struct ldb_context *ldb;
	int ret;
	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	ret = dsdb_set_global_schema(ldb);
	PyErr_LDB_ERROR_IS_ERR_RAISE(py_ldb_get_exception(), ret, ldb);

	Py_RETURN_NONE;
}

static PyObject *py_dsdb_load_partition_usn(PyObject *self, PyObject *args)
{
	PyObject *py_dn, *py_ldb, *result;
	struct ldb_dn *dn;
	uint64_t highest_uSN, urgent_uSN;
	struct ldb_context *ldb;
	TALLOC_CTX *mem_ctx;
	int ret;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
	   PyErr_NoMemory();
	   return NULL;
	}

	if (!PyArg_ParseTuple(args, "OO", &py_ldb, &py_dn)) {
	   talloc_free(mem_ctx);
	   return NULL;
	}

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	if (!PyObject_AsDn(mem_ctx, py_dn, ldb, &dn)) {
	   talloc_free(mem_ctx);
	   return NULL;
	}

	ret = dsdb_load_partition_usn(ldb, dn, &highest_uSN, &urgent_uSN);
	if (ret != LDB_SUCCESS) {
	   char *errstr = talloc_asprintf(mem_ctx, "Failed to load partition uSN - %s", ldb_errstring(ldb));
	   PyErr_SetString(PyExc_RuntimeError, errstr);
	   talloc_free(mem_ctx);
	   return NULL;
	}

	talloc_free(mem_ctx);

	result = PyDict_New();

	PyDict_SetItemString(result, "uSNHighest", PyInt_FromLong((uint64_t)highest_uSN));
	PyDict_SetItemString(result, "uSNUrgent", PyInt_FromLong((uint64_t)urgent_uSN));


	return result;
}

static PyObject *py_dsdb_set_am_rodc(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	bool ret;
	struct ldb_context *ldb;
	int py_val;

	if (!PyArg_ParseTuple(args, "Oi", &py_ldb, &py_val))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);
	ret = samdb_set_am_rodc(ldb, (bool)py_val);
	if (!ret) {
		PyErr_SetString(PyExc_RuntimeError, "set_am_rodc failed");
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *py_dsdb_set_schema_from_ldif(PyObject *self, PyObject *args)
{
	WERROR result;
	char *pf, *df;
	PyObject *py_ldb;
	struct ldb_context *ldb;

	if (!PyArg_ParseTuple(args, "Oss", &py_ldb, &pf, &df))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	result = dsdb_set_schema_from_ldif(ldb, pf, df);
	PyErr_WERROR_IS_ERR_RAISE(result);

	Py_RETURN_NONE;
}

static PyObject *py_dsdb_set_schema_from_ldb(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	struct ldb_context *ldb;
	PyObject *py_from_ldb;
	struct ldb_context *from_ldb;
	struct dsdb_schema *schema;
	int ret;
	if (!PyArg_ParseTuple(args, "OO", &py_ldb, &py_from_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	PyErr_LDB_OR_RAISE(py_from_ldb, from_ldb);

	schema = dsdb_get_schema(from_ldb, NULL);
	if (!schema) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to set find a schema on 'from' ldb!\n");
		return NULL;
	}

	ret = dsdb_reference_schema(ldb, schema, true);
	PyErr_LDB_ERROR_IS_ERR_RAISE(py_ldb_get_exception(), ret, ldb);

	Py_RETURN_NONE;
}

static PyObject *py_dsdb_write_prefixes_from_schema_to_ldb(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	struct ldb_context *ldb;
	WERROR result;
	struct dsdb_schema *schema;

	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	schema = dsdb_get_schema(ldb, NULL);
	if (!schema) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to set find a schema on ldb!\n");
		return NULL;
	}

	result = dsdb_write_prefixes_from_schema_to_ldb(NULL, ldb, schema);
	PyErr_WERROR_IS_ERR_RAISE(result);

	Py_RETURN_NONE;
}


static PyObject *py_dsdb_get_partitions_dn(PyObject *self, PyObject *args)
{
	struct ldb_context *ldb;
	struct ldb_dn *dn;
	PyObject *py_ldb, *ret;
	TALLOC_CTX *tmp_ctx;
	PyObject *mod;

	mod = PyImport_ImportModule("ldb");

	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	tmp_ctx = talloc_new(NULL);

	dn = samdb_partitions_dn(ldb, tmp_ctx);

	if (dn == NULL) {
		talloc_free(tmp_ctx);
		Py_RETURN_NONE;
	}
	ret = PyLdbDn_FromDn(dn);
	talloc_free(tmp_ctx);
	return ret;
}


/*
  call into samdb_rodc()
 */
static PyObject *py_dsdb_am_rodc(PyObject *self, PyObject *args)
{
	PyObject *py_ldb;
	struct ldb_context *ldb;
	int ret;
	bool am_rodc;

	if (!PyArg_ParseTuple(args, "O", &py_ldb))
		return NULL;

	PyErr_LDB_OR_RAISE(py_ldb, ldb);

	ret = samdb_rodc(ldb, &am_rodc);
	if (samdb_rodc(ldb, &am_rodc) != LDB_SUCCESS) {
		PyErr_SetString(PyExc_RuntimeError, ldb_errstring(ldb));
		return NULL;
	}

	return PyBool_FromLong(am_rodc);
}


static PyMethodDef py_dsdb_methods[] = {
	{ "_samdb_server_site_name", (PyCFunction)py_samdb_server_site_name,
		METH_VARARGS, "Get the server site name as a string"},
	{ "_dsdb_convert_schema_to_openldap",
		(PyCFunction)py_dsdb_convert_schema_to_openldap, METH_VARARGS, 
		"dsdb_convert_schema_to_openldap(ldb, target_str, mapping) -> str\n"
		"Create an OpenLDAP schema from a schema." },
	{ "_samdb_set_domain_sid", (PyCFunction)py_samdb_set_domain_sid,
		METH_VARARGS,
		"samdb_set_domain_sid(samdb, sid)\n"
		"Set SID of domain to use." },
	{ "_samdb_get_domain_sid", (PyCFunction)py_samdb_get_domain_sid,
		METH_VARARGS,
		"samdb_get_domain_sid(samdb)\n"
		"Get SID of domain in use." },
	{ "_samdb_ntds_invocation_id", (PyCFunction)py_samdb_ntds_invocation_id,
		METH_VARARGS, "get the NTDS invocation ID GUID as a string"},
	{ "_samdb_set_ntds_settings_dn", (PyCFunction)py_samdb_set_ntds_settings_dn,
		METH_VARARGS,
		"samdb_set_ntds_settings_dn(samdb, ntds_settings_dn)\n"
		"Set NTDS Settings DN for this LDB (allows it to be set before the DB fully exists)." },
	{ "_dsdb_get_oid_from_attid", (PyCFunction)py_dsdb_get_oid_from_attid,
		METH_VARARGS, NULL },
	{ "_dsdb_get_attid_from_lDAPDisplayName", (PyCFunction)py_dsdb_get_attid_from_lDAPDisplayName,
		METH_VARARGS, NULL },
	{ "_dsdb_set_ntds_invocation_id",
		(PyCFunction)py_dsdb_set_ntds_invocation_id, METH_VARARGS,
		NULL },
	{ "_samdb_ntds_objectGUID", (PyCFunction)py_samdb_ntds_objectGUID,
		METH_VARARGS, "get the NTDS objectGUID as a string"},
	{ "_dsdb_set_global_schema", (PyCFunction)py_dsdb_set_global_schema,
		METH_VARARGS, NULL },
	{ "_dsdb_load_partition_usn", (PyCFunction)py_dsdb_load_partition_usn,
		METH_VARARGS,
		"get uSNHighest and uSNUrgent from the partition @REPLCHANGED"},
	{ "_dsdb_set_am_rodc",
		(PyCFunction)py_dsdb_set_am_rodc, METH_VARARGS,
		NULL },
	{ "_am_rodc",
		(PyCFunction)py_dsdb_am_rodc, METH_VARARGS,
		NULL },
	{ "_dsdb_set_schema_from_ldif", (PyCFunction)py_dsdb_set_schema_from_ldif, METH_VARARGS,
		NULL },
	{ "_dsdb_set_schema_from_ldb", (PyCFunction)py_dsdb_set_schema_from_ldb, METH_VARARGS,
		NULL },
	{ "_dsdb_write_prefixes_from_schema_to_ldb", (PyCFunction)py_dsdb_write_prefixes_from_schema_to_ldb, METH_VARARGS,
		NULL },
	{ "_dsdb_get_partitions_dn", (PyCFunction)py_dsdb_get_partitions_dn, METH_VARARGS, NULL },
	{ NULL }
};

void initdsdb(void)
{
	PyObject *m;

	m = Py_InitModule3("dsdb", py_dsdb_methods, 
			   "Python bindings for the directory service databases.");
	if (m == NULL)
		return;

	/* "userAccountControl" flags */
	PyModule_AddObject(m, "UF_NORMAL_ACCOUNT",
					   PyInt_FromLong(UF_NORMAL_ACCOUNT));
	PyModule_AddObject(m, "UF_TEMP_DUPLICATE_ACCOUNT",
					   PyInt_FromLong(UF_TEMP_DUPLICATE_ACCOUNT));
	PyModule_AddObject(m, "UF_SERVER_TRUST_ACCOUNT",
					   PyInt_FromLong(UF_SERVER_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_WORKSTATION_TRUST_ACCOUNT",
					   PyInt_FromLong(UF_WORKSTATION_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_INTERDOMAIN_TRUST_ACCOUNT",
					   PyInt_FromLong(UF_INTERDOMAIN_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_PASSWD_NOTREQD",
					   PyInt_FromLong(UF_PASSWD_NOTREQD));
	PyModule_AddObject(m, "UF_ACCOUNTDISABLE",
					   PyInt_FromLong(UF_ACCOUNTDISABLE));

	PyModule_AddObject(m, "UF_SCRIPT", PyInt_FromLong(UF_SCRIPT));
	PyModule_AddObject(m, "UF_ACCOUNTDISABLE", PyInt_FromLong(UF_ACCOUNTDISABLE));
	PyModule_AddObject(m, "UF_00000004", PyInt_FromLong(UF_00000004));
	PyModule_AddObject(m, "UF_HOMEDIR_REQUIRED", PyInt_FromLong(UF_HOMEDIR_REQUIRED));
	PyModule_AddObject(m, "UF_LOCKOUT", PyInt_FromLong(UF_LOCKOUT));
	PyModule_AddObject(m, "UF_PASSWD_NOTREQD", PyInt_FromLong(UF_PASSWD_NOTREQD));
	PyModule_AddObject(m, "UF_PASSWD_CANT_CHANGE", PyInt_FromLong(UF_PASSWD_CANT_CHANGE));
	PyModule_AddObject(m, "UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED", PyInt_FromLong(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED));
	PyModule_AddObject(m, "UF_TEMP_DUPLICATE_ACCOUNT", PyInt_FromLong(UF_TEMP_DUPLICATE_ACCOUNT));
	PyModule_AddObject(m, "UF_NORMAL_ACCOUNT", PyInt_FromLong(UF_NORMAL_ACCOUNT));
	PyModule_AddObject(m, "UF_00000400", PyInt_FromLong(UF_00000400));
	PyModule_AddObject(m, "UF_INTERDOMAIN_TRUST_ACCOUNT", PyInt_FromLong(UF_INTERDOMAIN_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_WORKSTATION_TRUST_ACCOUNT", PyInt_FromLong(UF_WORKSTATION_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_SERVER_TRUST_ACCOUNT", PyInt_FromLong(UF_SERVER_TRUST_ACCOUNT));
	PyModule_AddObject(m, "UF_00004000", PyInt_FromLong(UF_00004000));
	PyModule_AddObject(m, "UF_00008000", PyInt_FromLong(UF_00008000));
	PyModule_AddObject(m, "UF_DONT_EXPIRE_PASSWD", PyInt_FromLong(UF_DONT_EXPIRE_PASSWD));
	PyModule_AddObject(m, "UF_MNS_LOGON_ACCOUNT", PyInt_FromLong(UF_MNS_LOGON_ACCOUNT));
	PyModule_AddObject(m, "UF_SMARTCARD_REQUIRED", PyInt_FromLong(UF_SMARTCARD_REQUIRED));
	PyModule_AddObject(m, "UF_TRUSTED_FOR_DELEGATION", PyInt_FromLong(UF_TRUSTED_FOR_DELEGATION));
	PyModule_AddObject(m, "UF_NOT_DELEGATED", PyInt_FromLong(UF_NOT_DELEGATED));
	PyModule_AddObject(m, "UF_USE_DES_KEY_ONLY", PyInt_FromLong(UF_USE_DES_KEY_ONLY));
	PyModule_AddObject(m, "UF_DONT_REQUIRE_PREAUTH", PyInt_FromLong(UF_DONT_REQUIRE_PREAUTH));
	PyModule_AddObject(m, "UF_PASSWORD_EXPIRED", PyInt_FromLong(UF_PASSWORD_EXPIRED));
	PyModule_AddObject(m, "UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION", PyInt_FromLong(UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION));
	PyModule_AddObject(m, "UF_NO_AUTH_DATA_REQUIRED", PyInt_FromLong(UF_NO_AUTH_DATA_REQUIRED));
	PyModule_AddObject(m, "UF_PARTIAL_SECRETS_ACCOUNT", PyInt_FromLong(UF_PARTIAL_SECRETS_ACCOUNT));

	/* "groupType" flags */
	PyModule_AddObject(m, "GTYPE_SECURITY_BUILTIN_LOCAL_GROUP",
					   PyInt_FromLong(GTYPE_SECURITY_BUILTIN_LOCAL_GROUP));
	PyModule_AddObject(m, "GTYPE_SECURITY_GLOBAL_GROUP",
					   PyInt_FromLong(GTYPE_SECURITY_GLOBAL_GROUP));
	PyModule_AddObject(m, "GTYPE_SECURITY_DOMAIN_LOCAL_GROUP",
					   PyInt_FromLong(GTYPE_SECURITY_DOMAIN_LOCAL_GROUP));
	PyModule_AddObject(m, "GTYPE_SECURITY_UNIVERSAL_GROUP",
					   PyInt_FromLong(GTYPE_SECURITY_UNIVERSAL_GROUP));
	PyModule_AddObject(m, "GTYPE_DISTRIBUTION_GLOBAL_GROUP",
					   PyInt_FromLong(GTYPE_DISTRIBUTION_GLOBAL_GROUP));
	PyModule_AddObject(m, "GTYPE_DISTRIBUTION_DOMAIN_LOCAL_GROUP",
					   PyInt_FromLong(GTYPE_DISTRIBUTION_DOMAIN_LOCAL_GROUP));
	PyModule_AddObject(m, "GTYPE_DISTRIBUTION_UNIVERSAL_GROUP",
					   PyInt_FromLong(GTYPE_DISTRIBUTION_UNIVERSAL_GROUP));

	/* "sAMAccountType" flags */
	PyModule_AddObject(m, "ATYPE_NORMAL_ACCOUNT",
					   PyInt_FromLong(ATYPE_NORMAL_ACCOUNT));
	PyModule_AddObject(m, "ATYPE_WORKSTATION_TRUST",
					   PyInt_FromLong(ATYPE_WORKSTATION_TRUST));
	PyModule_AddObject(m, "ATYPE_INTERDOMAIN_TRUST",
					   PyInt_FromLong(ATYPE_INTERDOMAIN_TRUST));
	PyModule_AddObject(m, "ATYPE_SECURITY_GLOBAL_GROUP",
					   PyInt_FromLong(ATYPE_SECURITY_GLOBAL_GROUP));
	PyModule_AddObject(m, "ATYPE_SECURITY_LOCAL_GROUP",
					   PyInt_FromLong(ATYPE_SECURITY_LOCAL_GROUP));
	PyModule_AddObject(m, "ATYPE_SECURITY_UNIVERSAL_GROUP",
					   PyInt_FromLong(ATYPE_SECURITY_UNIVERSAL_GROUP));
	PyModule_AddObject(m, "ATYPE_DISTRIBUTION_GLOBAL_GROUP",
					   PyInt_FromLong(ATYPE_DISTRIBUTION_GLOBAL_GROUP));
	PyModule_AddObject(m, "ATYPE_DISTRIBUTION_LOCAL_GROUP",
					   PyInt_FromLong(ATYPE_DISTRIBUTION_LOCAL_GROUP));
	PyModule_AddObject(m, "ATYPE_DISTRIBUTION_UNIVERSAL_GROUP",
					   PyInt_FromLong(ATYPE_DISTRIBUTION_UNIVERSAL_GROUP));

	/* "domainFunctionality", "forestFunctionality" flags in the rootDSE */
	PyModule_AddObject(m, "DS_DOMAIN_FUNCTION_2000",
					   PyInt_FromLong(DS_DOMAIN_FUNCTION_2000));
	PyModule_AddObject(m, "DS_DOMAIN_FUNCTION_2003_MIXED",
					   PyInt_FromLong(DS_DOMAIN_FUNCTION_2003_MIXED));
	PyModule_AddObject(m, "DS_DOMAIN_FUNCTION_2003",
					   PyInt_FromLong(DS_DOMAIN_FUNCTION_2003));
	PyModule_AddObject(m, "DS_DOMAIN_FUNCTION_2008",
					   PyInt_FromLong(DS_DOMAIN_FUNCTION_2008));
	PyModule_AddObject(m, "DS_DOMAIN_FUNCTION_2008_R2",
					   PyInt_FromLong(DS_DOMAIN_FUNCTION_2008_R2));

	/* "systemFlags" */
	PyModule_AddObject(m, "SYSTEM_FLAG_CR_NTDS_NC",
					PyInt_FromLong(SYSTEM_FLAG_CR_NTDS_NC));
	PyModule_AddObject(m, "SYSTEM_FLAG_CR_NTDS_DOMAIN",
					PyInt_FromLong(SYSTEM_FLAG_CR_NTDS_DOMAIN));
	PyModule_AddObject(m, "SYSTEM_FLAG_CR_NTDS_NOT_GC_REPLICATED",
					PyInt_FromLong(SYSTEM_FLAG_CR_NTDS_NOT_GC_REPLICATED));
	PyModule_AddObject(m, "SYSTEM_FLAG_SCHEMA_BASE_OBJECT",
					PyInt_FromLong(SYSTEM_FLAG_SCHEMA_BASE_OBJECT));
	PyModule_AddObject(m, "SYSTEM_FLAG_ATTR_IS_RDN",
					PyInt_FromLong(SYSTEM_FLAG_ATTR_IS_RDN));
	PyModule_AddObject(m, "SYSTEM_FLAG_DISALLOW_MOVE_ON_DELETE",
					PyInt_FromLong(SYSTEM_FLAG_DISALLOW_MOVE_ON_DELETE));
	PyModule_AddObject(m, "SYSTEM_FLAG_DOMAIN_DISALLOW_MOVE",
					PyInt_FromLong(SYSTEM_FLAG_DOMAIN_DISALLOW_MOVE));
	PyModule_AddObject(m, "SYSTEM_FLAG_DOMAIN_DISALLOW_RENAME",
					PyInt_FromLong(SYSTEM_FLAG_DOMAIN_DISALLOW_RENAME));
	PyModule_AddObject(m, "SYSTEM_FLAG_CONFIG_ALLOW_LIMITED_MOVE",
					PyInt_FromLong(SYSTEM_FLAG_CONFIG_ALLOW_LIMITED_MOVE));
	PyModule_AddObject(m, "SYSTEM_FLAG_CONFIG_ALLOW_MOVE",
					PyInt_FromLong(SYSTEM_FLAG_CONFIG_ALLOW_MOVE));
	PyModule_AddObject(m, "SYSTEM_FLAG_CONFIG_ALLOW_RENAME",
					PyInt_FromLong(SYSTEM_FLAG_CONFIG_ALLOW_RENAME));
	PyModule_AddObject(m, "SYSTEM_FLAG_DISALLOW_DELETE",
					PyInt_FromLong(SYSTEM_FLAG_DISALLOW_DELETE));

	/* Kerberos encryption type constants */
	PyModule_AddObject(m, "ENC_ALL_TYPES",
			   PyInt_FromLong(ENC_ALL_TYPES));
	PyModule_AddObject(m, "ENC_CRC32",
			   PyInt_FromLong(ENC_CRC32));
	PyModule_AddObject(m, "ENC_RSA_MD5",
			   PyInt_FromLong(ENC_RSA_MD5));
	PyModule_AddObject(m, "ENC_RC4_HMAC_MD5",
			   PyInt_FromLong(ENC_RC4_HMAC_MD5));
	PyModule_AddObject(m, "ENC_HMAC_SHA1_96_AES128",
			   PyInt_FromLong(ENC_HMAC_SHA1_96_AES128));
	PyModule_AddObject(m, "ENC_HMAC_SHA1_96_AES256",
			   PyInt_FromLong(ENC_HMAC_SHA1_96_AES256));

	PyModule_AddObject(m, "SEARCH_FLAG_ATTINDEX", PyInt_FromLong(SEARCH_FLAG_ATTINDEX));
	PyModule_AddObject(m, "SEARCH_FLAG_PDNTATTINDEX", PyInt_FromLong(SEARCH_FLAG_PDNTATTINDEX));
	PyModule_AddObject(m, "SEARCH_FLAG_ANR", PyInt_FromLong(SEARCH_FLAG_ANR));
	PyModule_AddObject(m, "SEARCH_FLAG_PRESERVEONDELETE", PyInt_FromLong(SEARCH_FLAG_PRESERVEONDELETE));
	PyModule_AddObject(m, "SEARCH_FLAG_COPY", PyInt_FromLong(SEARCH_FLAG_COPY));
	PyModule_AddObject(m, "SEARCH_FLAG_TUPLEINDEX", PyInt_FromLong(SEARCH_FLAG_TUPLEINDEX));
	PyModule_AddObject(m, "SEARCH_FLAG_SUBTREEATTRINDEX", PyInt_FromLong(SEARCH_FLAG_SUBTREEATTRINDEX));
	PyModule_AddObject(m, "SEARCH_FLAG_CONFIDENTIAL", PyInt_FromLong(SEARCH_FLAG_CONFIDENTIAL));
	PyModule_AddObject(m, "SEARCH_FLAG_NEVERVALUEAUDIT", PyInt_FromLong(SEARCH_FLAG_NEVERVALUEAUDIT));
	PyModule_AddObject(m, "SEARCH_FLAG_RODC_ATTRIBUTE", PyInt_FromLong(SEARCH_FLAG_RODC_ATTRIBUTE));

	PyModule_AddObject(m, "DS_FLAG_ATTR_NOT_REPLICATED", PyInt_FromLong(DS_FLAG_ATTR_NOT_REPLICATED));
	PyModule_AddObject(m, "DS_FLAG_ATTR_REQ_PARTIAL_SET_MEMBER", PyInt_FromLong(DS_FLAG_ATTR_REQ_PARTIAL_SET_MEMBER));
	PyModule_AddObject(m, "DS_FLAG_ATTR_IS_CONSTRUCTED", PyInt_FromLong(DS_FLAG_ATTR_IS_CONSTRUCTED));
}