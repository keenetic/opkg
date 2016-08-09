#! /usr/bin/env python3
#
# Reporter: Eric Yu
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Install a package 'x'
# 2. Install a package 'a' which depends on 'b' and 'c', where 'c' conflicts with 'x'
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Expected:
# 'x' remains installed, and 'a', 'b', and 'c' remain uninstalled because
# 'c' conflicts with 'x'
#
# Actual:
# 'x' remains installed. 'a' and 'c' are not installed, but 'b' is installed.
#
# Status
# ======
# Open
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x")
o.add(Package="a", Depends="b, c")
o.add(Package="b")
o.add(Package="c", Conflicts="x");


o.write_opk()
o.write_list()
opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
    opk.fail("Package 'x' installed but reports as not installed.")

opkgcl.install("a")
if opkgcl.is_installed("c"):
    opk.fail("Package 'c' installed but conflicts with x.")
if opkgcl.is_installed("a"):
    opk.fail("Package 'a' installed but dependency 'c' is not installed due to a conflict.")
if opkgcl.is_installed("b"):
    opk.fail("Package 'b' installed and orphaned.")

opkgcl.remove("x")
opkgcl.remove("a")
opkgcl.remove("b")
opkgcl.remove("c")
