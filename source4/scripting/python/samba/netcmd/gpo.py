#!/usr/bin/env python
#
# implement samba_tool gpo commands
#
# Copyright Andrew Tridgell 2010
#
# based on C implementation by Guenther Deschner and Wilco Baan Hofman
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import samba.getopt as options
import ldb

from samba.auth import system_session
from samba.netcmd import (
    Command,
    CommandError,
    Option,
    SuperCommand,
    )
from samba.samdb import SamDB
from samba import drs_utils, nttime2string, dsdb
from samba.dcerpc import misc


def samdb_connect(ctx):
    '''make a ldap connection to the server'''
    try:
        ctx.samdb = SamDB(url=ctx.url,
                          session_info=system_session(),
                          credentials=ctx.creds, lp=ctx.lp)
    except Exception, e:
        raise CommandError("LDAP connection to %s failed " % ctx.url, e)


def attr_default(msg, attrname, default):
    '''get an attribute from a ldap msg with a default'''
    if attrname in msg:
        return msg[attrname][0]
    return default


def flags_string(flags, value):
    '''return a set of flags as a string'''
    if value == 0:
        return 'NONE'
    ret = ''
    for (str, val) in flags:
        if val & value:
            ret += str + ' '
            value &= ~val
    if value != 0:
        ret += '0x%08x' % value
    return ret.rstrip()


def parse_gplink(gplink):
    '''parse a gPLink into an array of dn and options'''
    ret = []
    a = gplink.split(']')
    for g in a:
        if not g:
            continue
        d = g.split(';')
        if len(d) != 2 or not d[0].startswith("[LDAP://"):
            raise RuntimeError("Badly formed gPLink '%s'" % g)
        ret.append({ 'dn' : d[0][8:], 'options' : int(d[1])})
    return ret


class cmd_listall(Command):
    """list all GPOs"""

    synopsis = "%prog gpo listall"

    takes_optiongroups = {
        "sambaopts": options.SambaOptions,
        "versionopts": options.VersionOptions,
        "credopts": options.CredentialsOptions,
    }

    takes_options = [
        Option("-H", help="LDB URL for database or target server", type=str)
        ]

    def run(self, H=None, sambaopts=None,
            credopts=None, versionopts=None, server=None):

        self.url = H
        self.lp = sambaopts.get_loadparm()

        self.creds = credopts.get_credentials(self.lp)
        if not self.creds.authentication_requested():
            self.creds.set_machine_account(self.lp)

        samdb_connect(self)

        policies_dn = self.samdb.get_default_basedn()
        policies_dn.add_child(ldb.Dn(self.samdb, "CN=Policies,CN=System"))

        gpo_flags = [
            ("GPO_FLAG_USER_DISABLE", dsdb.GPO_FLAG_USER_DISABLE ),
            ( "GPO_FLAG_MACHINE_DISABLE", dsdb.GPO_FLAG_MACHINE_DISABLE ) ]

        try:
            msg = self.samdb.search(base=policies_dn, scope=ldb.SCOPE_ONELEVEL,
                                    expression="(objectClass=groupPolicyContainer)",
                                    attrs=['nTSecurityDescriptor', 'versionNumber', 'flags', 'name', 'displayName', 'gPCFileSysPath'])
        except Exception, e:
            raise CommandError("Failed to list policies in %s" % policies_dn, e)
        for m in msg:
            print("GPO          : %s" % m['name'][0])
            print("display name : %s" % m['displayName'][0])
            print("path         : %s" % m['gPCFileSysPath'][0])
            print("dn           : %s" % m.dn)
            print("version      : %s" % attr_default(m, 'version', '0'))
            print("flags        : %s" % flags_string(gpo_flags, int(attr_default(m, 'flags', 0))))
            print("")


class cmd_list(Command):
    """list GPOs for a user"""

    synopsis = "%prog gpo list <username>"

    takes_optiongroups = {
        "sambaopts": options.SambaOptions,
        "versionopts": options.VersionOptions,
        "credopts": options.CredentialsOptions,
    }

    takes_args = [ 'username' ]

    takes_options = [
        Option("-H", help="LDB URL for database or target server", type=str)
        ]

    def run(self, username, H=None, sambaopts=None,
            credopts=None, versionopts=None, server=None):

        self.url = H
        self.lp = sambaopts.get_loadparm()

        self.creds = credopts.get_credentials(self.lp)
        if not self.creds.authentication_requested():
            self.creds.set_machine_account(self.lp)

        samdb_connect(self)

        try:
            user_dn = self.samdb.search(expression='(&(samAccountName=%s)(objectclass=User))' % username)[0].dn
        except Exception, e:
            raise CommandError("Failed to find user %s" % username, e)

        # check if its a computer account
        try:
            msg = self.samdb.search(base=user_dn, scope=ldb.SCOPE_BASE, attrs=['objectClass'])[0]
            is_computer = 'computer' in msg['objectClass']
        except Exception, e:
            raise CommandError("Failed to find objectClass for user %s" % username, e)

        print("TODO: get user token")
        # token = self.samdb.get_user_token(username)

        gpos = []

        inherit = True
        dn = ldb.Dn(self.samdb, str(user_dn)).parent()
        while True:
            msg = self.samdb.search(base=dn, scope=ldb.SCOPE_BASE, attrs=['gPLink', 'gPOptions'])[0]
            if 'gPLink' in msg:
                glist = parse_gplink(msg['gPLink'][0])
                for g in glist:
                    if not inherit and not (g['options'] & dsdb.GPLINK_OPT_ENFORCE):
                        continue
                    if g['options'] & dsdb.GPLINK_OPT_DISABLE:
                        continue

                    print("TODO: access checking")
                    #if not samdb.access_check(secdesc, token, security.SEC_RIGHTS_FILE_READ):
                    #    continue

                    # check the flags on the GPO
                    flags = int(attr_default(self.samdb.search(base=g['dn'], scope=ldb.SCOPE_BASE, attrs=['flags'])[0], 'flags', 0))
                    if is_computer and (flags & dsdb.GPO_FLAG_MACHINE_DISABLE):
                        continue
                    if not is_computer and (flags & dsdb.GPO_FLAG_USER_DISABLE):
                        continue
                    gpos.append(g)

            # check if this blocks inheritance
            gpoptions = int(attr_default(msg, 'gPOptions', 0))
            if gpoptions & dsdb.GPO_BLOCK_INHERITANCE:
                inherit = False

            if dn == self.samdb.get_default_basedn():
                break
            dn = dn.parent()

        print("GPO's for user %s" % username)
        for g in gpos:
            print("\t%s" % g['dn'])


class cmd_gpo(SuperCommand):
    """GPO commands"""

    subcommands = {}
    subcommands["listall"] = cmd_listall()
    subcommands["list"] = cmd_list()