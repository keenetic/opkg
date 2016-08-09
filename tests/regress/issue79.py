#! /usr/bin/env python3
#
# Reporter: pixdamix
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Given three packages A, B, C
#	A depends on B
#	A failed postinst and is in unpacked state
#	C depends on B
#
# 2. Upgrade to a new version of C which do not depends on B anymore, and use
#    --autoremove
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# B should not be removed, bot opkg uninstall it.
#
#
# Status
# ======
#
# Fixed in r625.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b")
o.add(Package="b", Version="1.0")
o.add(Package="c", Version="1.0", Depends="b")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install("a")
opkgcl.install("c")

opkgcl.flag_unpacked("a")

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b")
o.add(Package="b", Version="1.0")
o.add(Package="c", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade("--autoremove")

if not opkgcl.is_installed("b", "1.0"):
	opk.fail("b has been removed even though a still depends on it")
