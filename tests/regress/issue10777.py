#! /usr/bin/env python3
#
# Reporter: markus.lehtonen@linux.intel.com
#
# What steps will reproduce the problem?
########################################
#
#  1.- Create package 'a', which RECOMMENDS 'b'
#  2.- Create package 'b'
#  3.- Create package 'c', which DEPENDS on 'd'
#  4.- Create package 'd', which CONFLICTS with 'b'
#  5.- Install 'a'
#  6.- Install 'c'
#
# What is the expected output? What do you see instead?
########################################
#
#  Package 'a', 'c' and 'd' should be installed (and b should
#  be removed). Instead the installaton of 'c' fails.
#

import os
import opk, cfg, opkgcl
import re

opk.regress_init()

o = opk.OpkGroup()
o.add(Package='a', Recommends='b')
o.add(Package='b')
o.add(Package='c', Depends='d')
o.add(Package='d', Conflicts='b')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install('a')
if not opkgcl.is_installed('a'):
    opk.fail("Package 'a' not installed but reports as being installed.")
if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' not installed but reports as being installed.")

opkgcl.install('c')
if not opkgcl.is_installed('c'):
    opk.xfail("[internalsolv] Package 'c' not installed but reports as being installed.")
if not opkgcl.is_installed('d'):
    opk.fail("Package 'c' not installed but reports as being installed.")
if not opkgcl.is_installed('a'):
    opk.fail("Package 'a' was incorrectly removed.")
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' was not removed, but conflicts with 'd'")
