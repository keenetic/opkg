#! /usr/bin/env python3
#
# Reporter: Eric Yu
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create packages 'x', 'a', 'b', and 'c' such that 'a' recommends 'b',
# 'b' recommends 'c', and 'c' conflicts 'x'.
# 2. Install 'x'.
# 3. Try to install 'a'.
#
# What is the expected output? What do you see instead?
# =====================================================
#
# 'a' and 'b' should be installed since 'c' is not an absolute dependency
# of 'b'.
#
# Instead only 'b' is installed and 'a' remains uninstalled.
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
o.add(Package="b", Recommends="c")
o.add(Package="c", Conflicts="x")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x");
if not opkgcl.is_installed("x"):
    opk.fail("Package 'x' installed but reports as not installed")

opkgcl.install("a");
if not opkgcl.is_installed("x"):
    opk.fail("Package 'x' removed when 'a' installed, but should not be removed.")
if not opkgcl.is_installed("b"):
    opk.xfail("[internalsolv/libsolv<0.6.28]Package 'b' not installed, but should be installed as a recommended pkg of 'a'.")
if opkgcl.is_installed("c"):
    opk.fail("Package 'c' installed but conflicts with x.")
if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' not installed even though 'c' is not an absolute dependency of 'b'.")
