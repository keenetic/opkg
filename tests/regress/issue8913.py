#! /usr/bin/env python3
#
# Reporter: alejandro.delcastillo@ni.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1.- Create package a (v 1.0) that Provides b and c, Replaces b, Conflicts with b.
#         install it
# 2.- Create package a (v 2.0) that Provides b and c, Replaces b, Conflicts with b.
#         upgrade
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Upgrade fails
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Provides="b, c", Replaces="b", Conflicts="b")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Provides="b, c", Replaces="b", Conflicts="b")
o.write_opk()
o.write_list()

opkgcl.update()
status = opkgcl.upgrade()

if not opkgcl.is_installed("a", "2.0"):
	opk.fail("New version of package 'a' available during upgrade but was not installed")

opkgcl.remove("a")
