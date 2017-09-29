#! /usr/bin/env python3
#
# Test opkg list-upgradable behavior.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version ="1.0")
o.add(Package="a", Version ="2.0")
o.add(Package="a", Version ="3.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a=1.0")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a', Version '1.0', failed to install.")

if opkgcl.is_upgradable("a", "2.0"):
	opk.fail("Package 'a', is upgradable to version '2.0', though a better candidate is available.")

if not opkgcl.is_upgradable("a", "3.0"):
	opk.fail("Package 'a', is not upgradable to version '3.0'.")

opkgcl.install("a=2.0")
if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a', Version '2.0', failed to install.")

if not opkgcl.is_upgradable("a", "3.0"):
	opk.fail("Package 'a', is not upgradable to version '3.0'.")

opkgcl.install("a=3.0")
if not opkgcl.is_installed("a", "3.0"):
	opk.fail("Package 'a', Version '3.0', failed to install.")

if opkgcl.is_upgradable("a"):
	opk.fail("Package 'a', is upgradable, but is already the latest.")
