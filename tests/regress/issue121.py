#! /usr/bin/env python3
#
# Reported by KirGeNe, Feb 6, 2014
# Test case modified by Paul Barker
#
# What steps will reproduce the problem?
# 1. Create package 'a' in repo, version 1.0, provides 'b'.
# 2. Create package 'x' in repo, depends on 'a'.
# 3. Create package 'y' in repo, depends on 'b'.
# 4. Install 'x' & 'y'.
# 5. Change 'a' in repo to version 2.0, no longer provides 'b'.
# 6. Create package 'b' in repo.
# 7. Do upgrade.
#
# What is the expected output?
# Packages 'a' v2.0 and 'b' v1.0 are installed.
#
# What do you see instead?
# Only package 'a' v2.0 is installed. Thus provider of 'b' is missing.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Provides="b")
o.add(Package="x", Depends="a")
o.add(Package="y", Depends="b")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x"):
	opk.fail("Package 'x' failed to install.")

opkgcl.install("y")
if not opkgcl.is_installed("y"):
	opk.fail("Package 'y' failed to install.")

if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' not installed despite being a dependency of installed packages.")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Depends="b")
o.add(Package="b")
o.add(Package="x", Depends="a")
o.add(Package="y", Depends="b")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("a", "2.0"):
	opk.fail("New version of package 'a' available during upgrade but was not installed")
if opkgcl.is_installed("a", "1.0"):
	opk.fail("Package 'a' upgraded but old version still installed.")

if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' not installed despite being a dependency of installed packages.")
