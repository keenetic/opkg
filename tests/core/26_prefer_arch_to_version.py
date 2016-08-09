#! /usr/bin/env python3
#
# Create package 'x(1.0)' with architecture a and 'x(2.0)' with architecture b.
# Install 'x' with --add-arch a:2 and --add-arch b:1.
# Check that 'x(2.0)' is installed. Then remove 'x'.
# Install 'x' with --add-arch a:2 and --add-arch b:1 and --prefer-arch-to-version.
# Check that 'x(1.0)' is installed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="x", Version="1.0", Architecture="a")
o.add(Package="x", Version="2.0", Architecture="b")

o.write_opk()
o.write_list()

opkgcl.update()

arch_flags = "--add-arch a:2 --add-arch b:1"

# install should prioritize version
opkgcl.install("x", arch_flags)
if opkgcl.is_installed("x", "1.0", arch_flags):
        opk.fail("Package 'x(2.0)' available but 1.0 installed")
if not opkgcl.is_installed("x", "2.0", arch_flags):
        opk.fail("Package 'x(2.0)' available but was not installed.")

opkgcl.remove("x", arch_flags)
if opkgcl.is_installed("x", "2.0", arch_flags):
        opk.fail("Package 'x' removed but reports as installed.")

# prefer-arch-to-version should prefer the architecture
opkgcl.install("x", arch_flags + " --prefer-arch-to-version")
if opkgcl.is_installed("x", "2.0", arch_flags):
    opk.fail("Package 'x(1.0) has preferred arch but 2.0 was installed")
if not opkgcl.is_installed("x", "1.0", arch_flags):
    opk.fail("Package 'x(1.0)' with preferred arch available but was not installed.")
