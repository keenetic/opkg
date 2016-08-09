#! /usr/bin/env python3
#
# Install version 1.0 of a package 'x' which PROVIDES and CONFLICTS 'v', then
# try to upgrade it.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Provides="v", Conflicts="v", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' installed but reports as not installed.")

# Now try to upgrade

o.add(Package="x", Provides="v", Conflicts="v", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("x", "2.0"):
	opk.fail("New version of package 'x' available during upgrade but was not installed")
if opkgcl.is_installed("x", "1.0"):
	opk.fail("Package 'a' upgraded but old version still installed.")
