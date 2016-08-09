#! /usr/bin/env python3
#
# Create packages 'a', 'b' which recommends 'a' and 'c' which depends on 'a'.
# Install 'b' with '--no-install-recommends' and ensure 'a' is not installed.
# Install 'c' with '--no-install-recommends' and ensure 'a' is installed.
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

opkgcl.install("b", "--no-install-recommends")
if opkgcl.is_installed("a"):
    opk.fail("Recommended package 'a' installed despite '--no-install-recommends'.")

opkgcl.install("c", "--no-install-recommends")
if not opkgcl.is_installed("a"):
    opk.fail("Required package 'a' not installed with '--no-install-recommends'.")
