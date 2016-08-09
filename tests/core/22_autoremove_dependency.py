#! /usr/bin/env python3
#
# Create packages 'a' and 'b' which both depend on 'c'.
# Install 'a' and 'b' and check that 'c' is autoinstalled.
# Remove 'a' with --autoremove and check that 'c' is not autoremoved.
# Remove 'b' with --autoremove and check that 'c' is autoremoved.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="c")
o.add(Package="b", Depends="c")
o.add(Package="c")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed("c"):
        opk.fail("Package 'c' should be installed as a dependency of 'a' but does not report as installed.")

opkgcl.install("b")
if not opkgcl.is_installed("b"):
	opk.fail("Package 'b' installed but does not report as installed.")

# Check the packages are marked correctly
if opkgcl.is_autoinstalled("a"):
	opk.fail("Package 'a' explicitly installed by user but reports as auto installed.")
if opkgcl.is_autoinstalled("b"):
        opk.fail("Package 'b' explicitly installed by user but reports as auto installed.")
if not opkgcl.is_autoinstalled("c"):
	opk.fail("Package 'c' installed as a dependency but does not report as auto installed.")

# Check that autoinstalled packages are not removed when other packages still depend on them
opkgcl.remove("a","--autoremove")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' removed but reports as installed.")
if not opkgcl.is_installed("c"):
        opk.fail("Package 'c' depended upon by 'b' but was autoremoved.")

# Check that autoinstalled packages are removed when no other packages depend on them
opkgcl.remove("b","--autoremove")
if opkgcl.is_installed("b"):
        opk.fail("Package 'b' removed but reports as installed.")
if opkgcl.is_installed("c"):
        opk.fail("Package 'c' not removed from --autoremove.")
