#! /usr/bin/env python3
#
# Reporter: Graham Gower
#
# What steps will reproduce the problem?
# ======================================
#
# Create two packages, with differing versions. Provide the newer package in an
# accessible repository. For this test I manually created an older
# abiword_1.6.8-r0.3 package (identical contents except the control file).
#
# $ ./opkg -o ~/opkg install abiword
# Installing abiword (2.6.8-r0.3) to root...
# Downloading http://10.0.0.13/ipk/mipsel/abiword_2.6.8-r0.3_mipsel.ipk
# Configuring abiword
# sh: rm: command not found
#
# $ ./opkg -o ~/opkg -force-downgrade install ~/opkg/abiword_1.6.8-r0.3_mipsel.ipk
# Downgrading abiword on root from 2.6.8-r0.3 to 1.6.8-r0.3...
# Configuring abiword
# sh: rm: command not found
#
# $ ./opkg -o ~/opkg -force-downgrade install ~/opkg/abiword_1.6.8-r0.3_mipsel.ipk
# Installing abiword (2.6.8-r0.3) to root...
# Downloading http://10.0.0.13/ipk/mipsel/abiword_2.6.8-r0.3_mipsel.ipk
# Configuring abiword
# sh: rm: command not found
#
# Note that it is now impossible to install the older version with opkg. To
# recover from this situation, the package information must be deleted from
# /usr/lib/opkg/status manually.
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# I expect the installation of the specified package to fail if it is already
# installed.
#
#
# Status
# ======
#
# Graham Gower:
# > I can no longer reproduce this. I suspect it has been fixed as a side effect
# > of r465.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="2.0")
o.write_opk()
o.write_list()

# older version, not in Packages list
a1 = opk.Opk(Package="a", Version="1.0")
a1.write()

opkgcl.update()

# install v2 from repository
opkgcl.install("a")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a_2.0' not installed.")

opkgcl.install("a_1.0_all.opk", "--force-downgrade")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a_1.0' not installed (1).")

opkgcl.install("a_1.0_all.opk", "--force-downgrade")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a_1.0' not installed (2).")

opkgcl.remove("a")
