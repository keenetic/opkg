#! /usr/bin/env python3
#
# Create package 'a' which depends on 'b'
# Install 'a' with --nodeps and check that 'b' is not installed
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b")

o.write_opk()
o.write_list()

opkgcl.update()

# installing with --nodeps should not install deps
opkgcl.install("a", "--nodeps")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed but reports as not installed")
if opkgcl.is_installed("b"):
        opk.fail("Package 'a' installed with --nodeps but dependency 'b' installed.")
