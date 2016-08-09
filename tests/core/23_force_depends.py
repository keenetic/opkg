#! /usr/bin/env python3
#
# Create package 'a' with a nonexistent dependency.
# Install 'a' with --force-depends and check that 'a' is installed.
#
# Create package 'b' which depends on 'c'
# Install 'b' with --force-depends and check that both 'b' and 'c' are installed.
# Remove 'c' with --force-depends and check that 'c' is removed
# while 'b' remains installed
#


import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="nonexistent")
o.add(Package="b", Depends="c")
o.add(Package="c");


o.write_opk()
o.write_list()

opkgcl.update()

# check that --force-depends works when installing a package
opkgcl.install("a","--force-depends")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a' installed with --force-depends but does not report as installed.")

# check that --force-depends does not stop deps from being installed when available
opkgcl.install("b", "--force-depends")
if not opkgcl.is_installed("b"):
    opk.fail("Package 'b' installed but does not report as installed.")
if not opkgcl.is_installed("c"):
    opk.fail("Package 'b' depends on 'c' and 'c' is available, but 'c' not installed.")

# check that --force-depends works when removing a package
opkgcl.remove("c", "--force-depends")
if opkgcl.is_installed("c"):
    opk.fail("Package 'c' removed with --force-depends but reports as still installed.")
if not opkgcl.is_installed("b"):
    opk.fail("Package 'b' not removed but reports as not installed.")
