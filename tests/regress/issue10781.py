#! /usr/bin/env python3
#
# Reporter: markus.lehtonen@linux.intel.com
#
# What steps will reproduce the problem?
########################################
#
# 1. Create package 'a', which DEPENDS on 'b'.
# 2. Create package 'b', which DEPENDS on 'd'.
# 3. Create package 'c', which PROVIDES 'd' and CONFLICTS with 'e''.
# 4. Create package 'e', which PROVIDES 'd' and CONFLICTS with 'c'.
# 5. Install 'a'
#
# What is the expected output? What do you see instead?
########################################
# Packages 'a', 'b' and 'c' or 'e' should be installed.
#
# Instead, opkg errors out:
#    Collected errors:
#     * check_conflicts_for: The following packages conflict with c:
#     * check_conflicts_for:         e*
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b", Depends="d")
o.add(Package="c", Provides="d", Conflicts="e")
o.add(Package="e", Provides="d", Conflicts="c")
o.write_opk()
o.write_list()

opkgcl.update()

status, stdout = opkgcl.opkgcl(" --force-postinstall install a")
if status != 0:
    opk.fail("Return value was different than 0")
if "Collected errors" in stdout:
    opk.fail("Command returned an error")
if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("b"):
    opk.fail("Package 'b' should be installed as a dependency of 'a' but does not report as installed.")
if not opkgcl.is_installed("c") and not opkgcl.is_installed("e"):
    opk.fail("Package 'c' or 'e' should be installed as a dependency of 'b' but does not report as installed.")
if opkgcl.is_installed("c") and opkgcl.is_installed("e"):
    opk.fail("Package 'c' or 'e' should be installed as a dependency of 'b' but not both.")
