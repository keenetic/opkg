#! /usr/bin/env python3
#
# Reporter: alexeytech@gmail.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create package a that have files /x1 /x2
#        install it
# 2. Create package b that have files /x2 /x3 and Replaces a, Provides a,
#    Conflicts a
#        install it
#
# What is the expected output? What do you see instead?
# =====================================================
#
# a is removed, b is installed as expected, BUT opkg files b shows only file
# /x3 and /x2 stays in filesystem untracked. I expect that package B must own
# file a/x2.
#
# Status: Accepted
#

import os
import opk, cfg, opkgcl

opk.regress_init()

x1 = "x1"
x2 = "x2"
x3 = "x3"

open(x1, "w").close()
open(x2, "w").close()
a = opk.Opk(Package="a")
a.write(data_files=[x1, x2])
os.unlink(x1)
os.unlink(x2)
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' not installed.")

open(x2, "w").close()
open(x3, "w").close()
a = opk.Opk(Package="b", Replaces="a", Provides="a", Conflicts="a")
a.write(data_files=[x2, x3])
os.unlink(x2)
os.unlink(x3)
opkgcl.install("b_1.0_all.opk")

if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' not installed.")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' was not replaced.")

x2fullpath = "{}/x2".format(cfg.offline_root)
if not x2fullpath in opkgcl.files("b"):
        opk.fail("Package 'b' does not own file 'x2'.")

opkgcl.remove("b")
