#! /usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
#
# Create package 'x(1.0)' with architecture a and 'x(1.0)' with architecture b.
# Install 'x' with --add-arch a:2 and --add-arch b:3.
# Check that 'x(1.0), arch b' is installed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Version="1.0", Architecture="b")
o.add(Package="x", Version="1.0", Architecture="a")

o.write_opk()
o.write_list()

opkgcl.update()

# install should pick arch=b version
opkgcl.install("x", "--add-arch a:2 --add-arch b:3")
if opkgcl.is_installed("x", "1.0", "--add-arch a:2"):
        opk.fail("Package 'x(1.0), arch b' available but arch a installed")
if not opkgcl.is_installed("x", "1.0", "--add-arch b:3"):
        opk.fail("Package 'x(1.0), arch=b' available but was not installed.")

opkgcl.install("x", "--force-reinstall --add-arch a:2 --add-arch b:3")
if not opkgcl.is_installed("x", "1.0", "--add-arch b:3"):
        opk.fail("Package 'x(1.0), arch=b' available but was not re-installed.")
