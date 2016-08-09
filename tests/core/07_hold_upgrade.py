#! /usr/bin/env python3
#
# Install a package and mark it as on hold. Make an updated version available
# and upgrade. Check that the new version is not installed and the old version
# remains installed.
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

opkgcl.flag_hold("a")

# Make a new version of 'a' available
o.add(Package="a", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if opkgcl.is_installed("a", "2.0"):
	opk.fail("Old version of package 'a' flagged as on hold but was upgraded.")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' not upgraded but old version was removed.")
