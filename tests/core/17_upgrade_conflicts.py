#! /usr/bin/env python3
#
# Install a package A at version V. Install a package B, which conflicts
# with a version of A older than V. Make available A at a version newer
# than V. Do upgrade and ensure that A gets installed correctly.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="2.0")
o.add(Package="b", Version="1.0", Conflicts="a (<< 1.0)")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
opkgcl.install("b")

o = opk.OpkGroup()
o.add(Package="a", Version="3.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("a", "3.0"):
	opk.fail("New version of package 'a' available during upgrade but was not installed")
