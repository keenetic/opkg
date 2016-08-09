#! /usr/bin/env python3
#
# Create essential packages 'a(1.0)' and 'a(2.0)'.
# Install 'a(1.0)'.
# Remove 'a' and check that 'a' is not removed because it is essential
# Upgrade 'a' to version (2.0) and check that the upgrade works
# Remove 'a' and check that 'a' is not removed because it is essential
# Remove 'a' with --force-removal-of-essential-packages and check that it is removed
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Essential="yes")
o.add(Package="a", Version = "2.0", Essential="yes")

o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a_1.0_all.opk")
if not opkgcl.is_installed("a","1.0"):
        opk.fail("Package 'a' installed but does not report as installed.")

opkgcl.remove("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a 1.0' is marked essential but was removed.")

# Upgrading a should be allowed
opkgcl.install("a_2.0_all.opk")
if not opkgcl.is_installed("a","2.0"):
        opk.fail("Package 'a 2.0' available but was not upgraded.")
if opkgcl.is_installed("a","1.0"):
        opk.fail("Package 'a' upgraded but old version not removed.")

opkgcl.remove("a")
if not opkgcl.is_installed("a"):
        opk.fail("Package 'a 2.0' is marked essential but was removed.")

opkgcl.remove("a", "--force-removal-of-essential-packages")
if opkgcl.is_installed("a"):
        opk.fail("Package 'a' was removed with"
                 " --force-removal-of-essential-packages but reports as installed")
