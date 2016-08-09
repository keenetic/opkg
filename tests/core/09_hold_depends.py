#! /usr/bin/env python3
#
# Mark an installed package a-1.0 as on hold, make available a-2.0 depended on
# by z. Check that z refuses to install due to a-1.0 being held.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' installed but reports as not installed.")

opkgcl.flag_hold("x")

# Make new versions of 'x' and 'z' available
o.add(Package="x", Version="2.0")
o.add(Package="z", Depends="x>=2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install("z")

if opkgcl.is_installed("z"):
	opk.fail("Package 'z' installed despite dependency on 'x>=2.0' with 'x' version 1.0 on hold.")
if opkgcl.is_installed("x", "2.0"):
	opk.fail("Old version of package 'x' flagged as on hold but was upgraded.")
if not opkgcl.is_installed("x", "1.0"):
	opk.fail("Package 'x' not upgraded but old version was removed.")
