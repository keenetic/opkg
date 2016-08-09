#! /usr/bin/env python3
#
# Install a package 'x' which PROVIDES 'v' and ensure that we can install
# another package 'y' which also PROVIDES 'v'.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Provides="v")
o.add(Package="y", Provides="v")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' installed but reports as not installed.")

# Now try to install "y", which should succeed

opkgcl.install("y")
if not opkgcl.is_installed("y"):
	opk.fail("Package 'y' not installed despite lack of conflicts.")
