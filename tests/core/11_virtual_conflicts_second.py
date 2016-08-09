#! /usr/bin/env python3
#
# Install a package 'x' which PROVIDES 'v'. Then try to install 'y' which
# PROVIDES and CONFLICTS 'v', indicating that no other provider of the virtual
# package 'v' should be installed at the same time as 'y'.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Provides="v")
o.add(Package="y", Provides="v", Conflicts="v")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' installed but reports as not installed.")

# Now try to install "y", which should fail

opkgcl.install("y")
if opkgcl.is_installed("y"):
	opk.xfail("[internalsolv] Package 'y' installed despite conflict with 'v' provided by 'x'.")
