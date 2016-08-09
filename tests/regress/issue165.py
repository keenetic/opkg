#! /usr/bin/env python3
#
# Reporter: Eric Yu
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create packages 'x', 'a', and 'b' such that 'a' recommends 'b'
#    and 'b' conflicts 'x'.
# 2. Install 'x'.
# 3. Try to install 'a'.
#
# What is the expected output? What do you see instead?
# =====================================================
#
# 'b' should remain uninstalled, but 'a' should be installed because 'b' is not
# an absolute dependency of 'a'.
#
# Instead nothing is  installed.
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
o.add(Package="a", Recommends="b")
o.add(Package="b", Conflicts="x")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x");
if not opkgcl.is_installed("x"):
    opk.fail("Package 'x' installed but reports as not installed")

opkgcl.install("a");
if not opkgcl.is_installed("x"):
    opk.fail("Package 'x' removed when 'a' installed, but should not be removed.")
if opkgcl.is_installed("b"):
    opk.fail("Package 'b' installed but conflicts with x.")
if not opkgcl.is_installed("a"):
    opk.xfail("[internalsolv] Package 'a' not installed even though 'b' is not an absolute dependency of 'a'.")
