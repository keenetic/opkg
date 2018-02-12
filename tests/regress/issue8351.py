#! /usr/bin/env python3
#
# Reporter: muusclaus
#
# What steps will reproduce the problem?
########################################
#
#  1.- Create package 'y' (1.0), which Depends on 'x'
#  2.- Create package 'x' (1.0)
#  3.- Update
#  4.- Remove package x
#  5.- Install 'y'. The operation will fail, but will install
#      'y'
#  6.- Create package 'x' (2.0)
#  7.- Install 'x'
#
# What is the expected output? What do you see instead?
########################################
#
#  Package 'x' (2.0) should be installed
#  Instead, opkg tries to install 'x' (1.0), and fails.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
x = o.add(Package="x", Version="1.0")
o.add(Package="y", Version="1.0", Depends="x")

o.write_opk()
o.write_list()

opkgcl.update()

# Will force failure to install 'x', even though it's listed in Packages
os.unlink("x_1.0_all.opk")

opkgcl.install("y")

o.removeOpk(x)
o.add(Package="x", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("x")
if not opkgcl.is_installed("x", version="2.0"):
    opk.fail("Failed to install 'x' (2.0)")

opkgcl.remove("y")
if opkgcl.is_installed("y"):
    opk.fail("Failed to uninstall 'y'")

opkgcl.remove("x")
if opkgcl.is_installed("x"):
    opk.fail("Failed to uninstall 'x'")
