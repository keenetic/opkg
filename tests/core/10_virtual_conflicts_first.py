#! /usr/bin/env python3
#
# Install a package 'x' which PROVIDES and CONFLICTS 'v', indicating that 'v' is
# a virtual package and no other provider of this virtual package should be
# installed whilst 'x' is installed. Then try to install 'y' which also PROVIDES
# 'v'.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Provides="v", Conflicts="v")
o.add(Package="y", Provides="v")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' installed but reports as not installed.")

# Now try to install "y", which should fail

opkgcl.install("y")
if opkgcl.is_installed("y"):
	opk.fail("Package 'y' installed despite conflict with 'v' provided by 'x'.")
