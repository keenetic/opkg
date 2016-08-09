#! /usr/bin/env python3
#
# Reporter: Paul Barker
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create packages A and C which both recommend B.
# 2. Create package B.
# 3. Install A and C.
# 4. Remove A with --autoremove
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Expect B to remain installed as it is still depended on by C. Instead, B is
# removed.
#
# This test case was initially part of issue58.py but the failure was not as
# described in issue 58 in the bug tracker, so this was split out.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Recommends="b")
o.add(Package="b")
o.add(Package="c", Recommends="b")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
opkgcl.install("c")

opkgcl.remove("a", "--autoremove")
if not opkgcl.is_installed("b"):
	opk.fail("Pacakge 'b' removed despite remaining "
			"recommending package 'c'.")
