#! /usr/bin/env python3
#
# Create package 'a' which depends on 'b' and 'c'.
# Explicitly install 'b' then install 'a'.
# Check that 'c' is autoinstalled while 'a' and 'b' are not.
# Remove 'a' with --autoremove and check that 'c' is autoremoved while 'b' is not
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b, c")
o.add(Package="b")
o.add(Package="c")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("b")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' installed but does not report as installed.")

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("c"):
	opk.fail("Package 'c' should be installed as a dependency of 'a' but does not report as installed.")

# Check the packages are marked correctly
if opkgcl.is_autoinstalled("a"):
	opk.fail("Package 'a' explicitly installed but reports as auto installed.")
if opkgcl.is_autoinstalled("b"):
        opk.fail("Package 'b' explicitly installed but reports as auto installed.")
if not opkgcl.is_autoinstalled("c"):
	opk.fail("Package 'c' installed as a dependency but does not report as auto installed.")

# Check that autoinstalled packages are removed properly
opkgcl.remove("a","--autoremove")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' removed but reports as installed.")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' explicitly installed but was removed in autoremove.")
if opkgcl.is_installed("c"):
        opk.fail("Package 'c' installed as a dependency but was not autoremoved.")
