#! /usr/bin/env python3
#
# Reported by ccospel, Jan 27, 2012
#
# What steps will reproduce the problem?
#   1. Create two packages that provide the same virtual package.
#      - Package A should provide X
#      - Package B should provide X, replace X, conflict X
#   2. Install package A.  Then install package B.
#   3. Package B will be installed successfully, but package A will not have
#      first been removed. If doing an "opkg list", both packages appear and
#      package A files still exist in the file system.
#
# What is the expected output? What do you see instead?
#   I expect package A to first be removed, and then for package B to be
#   installed.  Instead, it remains and its files are overwritten by package
#   B's.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Provides="x")
o.add(Package="b", Provides="x", Replaces="x", Conflicts="x")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")

opkgcl.install("b")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' failed to install.")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' remains installed despite conflicts.")
