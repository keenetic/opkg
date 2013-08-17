#!/usr/bin/python3
#
# Reporter: Graham Gower
#
# What steps will reproduce the problem?
# ======================================
# 
# 1. Create package A which recommends B.
# 2. Create package B.
# 3. Install A.
# 4. Remove A with --autoremove 
#
#
# What is the expected output? What do you see instead?
# =====================================================
# 
# Expect B to be removed with A. Instead only A is removed.
#
#
# Status
# ======
#
# Fixed with r555/r556.

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
	opk.fail("Pacakge 'b' orphaned despite remaining "
			"recommending package 'c'.")

opkgcl.remove("c", "--autoremove")
if opkgcl.is_installed("b"):
	opk.fail("Recommended package 'b' not autoremoved.")
