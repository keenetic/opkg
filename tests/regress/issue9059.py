#! /usr/bin/env python3
#
# Reporter: scot.salmon@ni.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1.- Create package 'a', which PROVIDES and CONFLICTS 'x'
# 2.- Create package 'b', which DEPENDS on 'x'
# 3.- Create package 'x'
# 4.- Install 'a'
# 5.- Install 'b'
#
# What is the expected output? What do you see instead?
# ======================================
#
# Installation of b fails (it shouldn't)
#

import os
import opk, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Provides="x", Conflicts="x")
o.add(Package="b", Depends="x")
o.add(Package="x")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")

opkgcl.install("b")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' installed but reports as not installed.")
