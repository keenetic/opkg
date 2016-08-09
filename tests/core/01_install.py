#! /usr/bin/env python3
#
# Install a package and check that it is listed as installed. Then remove the
# package and check that it is no longer listed as installed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a")
o.write_opk()
o.write_list()

opkgcl.update()

if opkgcl.is_installed("a"):
	opk.fail("Package 'a' not installed but reports as being installed.")

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")

opkgcl.remove("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' removed but still reports as being installed.")
