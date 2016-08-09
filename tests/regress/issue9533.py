#! /usr/bin/env python3
#
# Reported: alejandro.delcastillo@ni.com
#
# What steps will reproduce the problem?
########################################
#   1. Create package 'a', which PROVIDES and CONFLICTS 'b'
#   2. Create package 'b'.
#   3. Install 'b'
#   4. Install 'a'
#
# What is the expected output? What do you see instead?
########################################
#
#   Installation of 'a' should fail, since 'a' conflicts with 'b'.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Provides="b", Conflicts="b")
o.add(Package="b")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install("b")

if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' failed to install.")

opkgcl.install("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' was incorrectly installed, since it conflicts with b.")
