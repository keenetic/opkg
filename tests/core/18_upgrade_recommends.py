#! /usr/bin/env python3
#
# Install a package B, which recommends package A. Verify that A installs
# automatically and gets flagged as auto-installed. Make a newer version
# of A and B available. Do upgrade with --autoremove option and ensure
# that A gets uninstalled correctly instead of being upgraded.
#
# This test case matches issue 144.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0")
o.add(Package="b", Version="1.0", Recommends="a")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("b")
if not opkgcl.is_installed("a"):
	opk.fail("Package a should have been auto-installed")
if not opkgcl.is_autoinstalled("a"):
	opk.fail("Package a should have been marked as auto-installed")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0")
o.add(Package="b", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade("b", "--autoremove")

if opkgcl.is_installed("a"):
	opk.fail("Package a should have been auto-removed")
