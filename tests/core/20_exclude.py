#! /usr/bin/env python3
#
# Create packages 'a', 'b' which recommends 'a' and 'c' which depends on 'a'.
# Install 'b' with '--add-exclude a' and ensure 'b' is installed but 'a' is not.
# Install 'c' with '--add-exclude a' and ensure 'c' and 'a' are not installed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a")
o.add(Package="b", Recommends="a")
o.add(Package="c", Depends="a")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("b", "--add-exclude a")
if opkgcl.is_installed("a"):
    opk.fail("Package 'a' installed despite being excluded.")
if not opkgcl.is_installed("b"):
    opk.fail("Package 'b' not installed, excluded pacakge was only a recommendation.")

opkgcl.install("c", "--add-exclude a")
if opkgcl.is_installed("a"):
    opk.fail("Package 'a' installed despite being excluded.")
if opkgcl.is_installed("c"):
    opk.fail("Package 'c' installed despite required package being excluded.")
