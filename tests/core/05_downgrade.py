#! /usr/bin/env python3
#
# Make available two versions of the same package and install it. Check that the
# latest version is chosen out of the two available. Then explicitly install the
# old version and check that it replaces the newer version, resulting in a
# downgrade.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0")
o.add(Package="a", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a' installed but latest version does not report as installed.")
if opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' version '1.0' installed as well as version '2.0'.")

# Explicitly install old version
opkgcl.install("a_1.0_all.opk", "--force-downgrade")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' explicitly downgraded but old version does not report as installed.")
if opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a' explicitly downgraded but latest version still reports as installed.")
