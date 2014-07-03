#! /usr/bin/env python
#
# Reported by radolin as a comment on issue 89.
#
# Steps:
#   1. Install A. Install B
#   2. Upgrade B to new version, which 'replaces: A'
#
# Expected result:
#   A is removed during B upgrade
#
# Actual Result:
#   Both A and new version of B are installed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a")
o.add(Package="b", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")

opkgcl.install("b")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' failed to install.")

o = opk.OpkGroup()
o.add(Package="a")
o.add(Package="b", Version="2.0", Replaces="a")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("b", "2.0"):
	opk.fail("New version of package 'b' available during upgrade but was not installed")
if opkgcl.is_installed("b", "1.0"):
	opk.fail("Package 'b' upgraded but old version still installed.")
if opkgcl.is_installed("a"):
	opk.xfail("Package 'a' remains installed despite being replaced by 'b'.")
