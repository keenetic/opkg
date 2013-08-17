#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="2.0")
o.write_opk()
o.write_list()

# older version, not in Packages list
a1 = opk.Opk(Package="a", Version="1.0")
a1.write()

opkgcl.update()

# install v2 from repository
opkgcl.install("a")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a_2.0' not installed.")

opkgcl.install("a_1.0_all.opk", "--force-downgrade")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a_1.0' not installed (1).")

opkgcl.install("a_1.0_all.opk", "--force-downgrade")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a_1.0' not installed (2).")

opkgcl.remove("a")
