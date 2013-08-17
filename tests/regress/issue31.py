#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b", Depends="c")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed, despite dependency "
			"upon a package with an unresolved dependency.")

if opkgcl.is_installed("b"):
	opk.fail("Package 'b' installed, "
			"despite unresolved dependency.")
