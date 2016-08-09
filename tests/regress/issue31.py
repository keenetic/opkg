#! /usr/bin/env python3
#
# Reporter: Graham Gower
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create a package, A.opk, with unresolved dependencies.
# 2. Create another package, B.opk, that depends upon A.
# 3. opkg install B.opk
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# A is installed and can be found in the status file.
#
#
# Status
# ======
#
# Graham Gower:
# > Can no longer reproduce. Must have been fixed as a side effect of other
# > changes...

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b", Depends="c")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed, despite dependency "
			"upon a package with an unresolved dependency.")

if opkgcl.is_installed("b"):
	opk.fail("Package 'b' installed, "
			"despite unresolved dependency.")
