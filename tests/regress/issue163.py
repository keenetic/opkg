#! /usr/bin/env python3
#
# Reporter: Eric Yu
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create packages 'a' and 'b', where 'a' depends on 'b'.
# 2. Install 'a' (which also installs 'b')
# 3. Remove 'b' with --force-removal-of-dependent-packages
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Removing 'b' with --force-removal-of-dependent-packages should remove
# both 'b' and 'a'.
#
# Instead, only 'b' is removed, leaving 'a' installed.
#
# Status
# ======
# Open
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed but reports as not installed")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' should be installed as a dependency of 'a' "
                 "but reports as not installed")

opkgcl.remove("b","--force-removal-of-dependent-packages")
if opkgcl.is_installed("b"):
        opk.fail("Package 'b' removed but reports as installed.")
if opkgcl.is_installed("a"):
        opk.fail("Package 'b' removed with --force-removal-of-dependent-packages "
                  "but 'a' which depends on 'b' still installed.")
