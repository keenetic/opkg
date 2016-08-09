#! /usr/bin/env python3
#
# Reported by alejandro.delcastillo@ni.com
#
# What steps will reproduce the problem?
########################################
#   1. Create package 'a', which DEPENDS on 'b'
#   2. Create package 'b', which DEPENDS on 'a'
#   3. Install 'a'
#
# What is the expected output? What do you see instead?
#######################################################
#   I expect package 'a' and 'b' to be installed, and only package 'b' to be set as Autoinstalled.
#   Instead, both package 'a' and 'b' are set to Autoinstalled.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b", Depends="a")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' should be installed as a dependency of 'a' but does not report as installed.")
if opkgcl.is_autoinstalled("a"):
        opk.fail("Package 'a' explicitly installed by reports as auto installed.")
if not opkgcl.is_autoinstalled("b"):
        opk.fail("Package 'b' installed as a dependency but does not report as auto installed.")
