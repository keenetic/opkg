#! /usr/bin/env python3
#
# Install a package and then make available a new version of the same package.
# Upgrade and check that the new version is installed and the old version
# removed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")

# Make a new version of 'a' available
o.add(Package="a", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("a", "2.0"):
	opk.fail("New version of package 'a' available during upgrade but was not installed")
if opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' upgraded but old version still installed.")
