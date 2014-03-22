#! /usr/bin/env python
#
# Issue 124, reported by muusclaus
#
# Install package 'a' version 1.0 which depends on package 'b' version 1.0. Then
# make available 'a' version 1.1 which depends on 'b' version 1.1. Attempt to
# upgrade 'b' to version 1.1.
#
# The upgrade should be blocked as the installed version of 'a' (1.0) depends
# explictly on 'b' version 1.0 and this dependency is not satisfied by 'b'
# version 1.1.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b (= 1.0)")
o.add(Package="b", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' not installed despite dependency from package 'a'.")

o = opk.OpkGroup()
o.add(Package="a", Version="1.1", Depends="b (= 1.1)")
o.add(Package="b", Version="1.1")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.upgrade("b")

# Check 'a' has not been upgraded
if opkgcl.is_installed("a", "1.1"):
	opk.fail("Package 'a' upgraded despite not being listed in packages to upgrade.")
if not opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' version 1.0 removed.")

# Check 'b' has not been upgraded
if opkgcl.is_installed("b", "1.1"):
	opk.xfail("Package 'b' upgraded despite breaking dependency of package 'a'.")
if not opkgcl.is_installed("b", "1.0"):
	opk.fail("Package 'b' version 1.0 removed.")
