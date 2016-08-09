#! /usr/bin/env python3
#
# Install a package 'a' which depends on a second package 'b'. Check that both
# are installed, that package 'a' does not report as automatically installed but
# that package 'b' does report as automatically installed.
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

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' should be installed as a dependency of 'a' but does not report as installed.")

# Check the packages are marked correctly
if opkgcl.is_autoinstalled("a"):
	opk.fail("Package 'a' explicitly installed by reports as auto installed.")
if not opkgcl.is_autoinstalled("b"):
	opk.fail("Package 'b' installed as a dependency but does not report as auto installed.")

# Check that trying to remove 'b' fails and doesn't change the system (issue 9862)
if not opkgcl.remove("b"):
        opk.fail("Package 'b' should not be allowed to be uninstalled")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' was incorrectly uninstalled")
if not opkgcl.is_installed("b"):
        opk.fail("Package 'b' was incorrectly uninstalled")
