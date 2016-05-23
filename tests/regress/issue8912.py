#! /usr/bin/env python3
#
# Reporter: alejandro.delcastillo@ni.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1.- Create package a, which depends on b and c
# 2.- Create package b
# 3.- Create package c, which depends on b
# 4.- Install a
# 5.- Remove a with --autoremove
#
# What is the expected output? What do you see instead?
# =====================================================
#
# All packages should be removed. Instead, b is left behind.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b, c")
o.add(Package="b")
o.add(Package="c", Depends="b")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' should be installed as a dependency of 'a' but does not report as installed.")
if not opkgcl.is_installed("c"):
        opk.fail("Package 'c' should be installed as a dependency of 'a' but does not report as installed.")

# Check the packages are marked correctly
if opkgcl.is_autoinstalled("a"):
	opk.fail("Package 'a' explicitly installed by user but reports as auto installed.")
if not opkgcl.is_autoinstalled("b"):
	opk.fail("Package 'b' installed as a dependency but does not report as auto installed.")
if not opkgcl.is_autoinstalled("c"):
	opk.fail("Package 'c' installed as a dependency but does not report as auto installed.")

# Check that autoinstalled packages are removed properly
opkgcl.remove("a","--autoremove")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' removed but reports as installed.")
if opkgcl.is_installed("b"):
        opk.xfail("[internalsolv] Package 'b' not removed from --autoremove.")
if opkgcl.is_installed("c"):
        opk.fail("Package 'c' not removed from --autoremove.")
