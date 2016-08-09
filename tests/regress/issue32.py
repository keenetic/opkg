#! /usr/bin/env python3
#
# Reporter: pixdamix@gmail.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Install a package A which depends on B
# 2. Update the package A which now depends on C, but C conflicts with B
# 3. Upgrade
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Expected:
#
# root@terminal:# opkg upgrade -force-defaults -autoremove
# Upgrading A on root from 1.0-r0.1 to 2.0-r0.1...
# Downloading ...
# Installing C (1.0-r0) to root...
# The following packages conflict with C: B
# B is now orphaned, uninstalling...
#
# Actual:
#
# root@terminal:# opkg upgrade -force-defaults -autoremove
# Upgrading A on root from 1.0-r0.1 to 2.0-r0.1...
# Downloading ...
# Installing C (1.0-r0) to root...
# Collected errors: * ERROR: The following packages conflict with C: *
#
# Status: Accepted
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b")
o.add(Package="b")
o.write_opk()
o.write_list()
opkgcl.update()
opkgcl.install("a")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Depends="c")
o.add(Package="c", Conflicts="b")
o.write_opk()
o.write_list()

opkgcl.update()
status = opkgcl.upgrade("a", "--autoremove")

if not opkgcl.is_installed("a", "2.0"):
    opk.xfail("[internalsolv] New version of package 'a' available during upgrade but was not installed")
if status != 0:
    opk.fail("Return value was different than 0")

opkgcl.remove("a")
opkgcl.remove("b")
opkgcl.remove("c")
