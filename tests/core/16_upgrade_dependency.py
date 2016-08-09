#! /usr/bin/env python3
#
# Install a package 'a' at version 1.0. Make available 'a' at version 2.0 with
# dependency on 'b', and make available 'b'. Do upgrade and ensure that 'b' in
# installed correctly.
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

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Depends="b")
o.add(Package="b")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("a", "2.0"):
	opk.fail("New version of package 'a' available during upgrade but was not installed")
if opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' upgraded but old version still installed.")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' should be installed as a dependency of 'a' but does not report as installed.")

# Check the packages are marked correctly
if opkgcl.is_autoinstalled("a"):
	opk.fail("Package 'a' explicitly installed by reports as auto installed.")
if not opkgcl.is_autoinstalled("b"):
	opk.fail("Package 'b' installed as a dependency but does not report as auto installed.")
