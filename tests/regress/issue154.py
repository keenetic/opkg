#! /usr/bin/env python3
#
# Reporter: michal.p...@gmail.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Have package A (1.0)  depending on package B (1.0).
# 2. On the repo, add package A (2.0) and pacakge B (2.0)
# 3. Do 'opkg-cl upgrade'
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# opkg should upgrade package A and B to version 2 and exit with 0.
# It correctly updates the packages, but exits with -1.
#
# Status: Resolved
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b")
o.add(Package="b", Version="1.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install("a")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Depends="b")
o.add(Package="b", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
status = opkgcl.upgrade()
if not opkgcl.is_installed("a", "2.0"):
    opk.fail("New version of package 'a' available during upgrade but was not installed")
if not opkgcl.is_installed("b", "2.0"):
    opk.fail("New version of package 'b' available during upgrade but was not installed")
if status != 0:
    opk.fail("Return value was different than 0")
opkgcl.remove("a")
opkgcl.remove("b")
