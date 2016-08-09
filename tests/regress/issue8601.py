#! /usr/bin/env python3
#
# Reporter: jatinvatsalya@gmail.com
#
# What steps will reproduce the problem?
########################################
#   1. Create package 'a', version 2.0.
#   2. Create package 'b', which DEPENDS on 'a', version 1.0
#   3. Install 'a' and 'b' together (b won't install)
#   4. Remove 'a'
#   4. Install 'a'
#
# What is the expected output? What do you see instead?
########################################
#   The first install operation should install 'a', but fail on 'b'. The
#   second install operation should succeed, but instead it fails.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="2.0")
o.add(Package="b", Depends="a (=1.0)")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a b")
# Using the internal solver, 'a' does get installed, but using libsolv, it doesn't.
# Skipping check for 'a' status so the test will work with both solvers.
# Regarding 'b', independently of the backend, it shouldn't be installed
if opkgcl.is_installed("b"):
        opk.fail("Package 'b' installed despite missing dependency on 'a' (=1.0).")

opkgcl.remove("a")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' removed but still reports as being installed")

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")
