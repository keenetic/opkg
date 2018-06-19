#! /usr/bin/env python3
#
# Create essential packages 'a', which Depends on b. Package 'b' depends on 'c'
# Install 'a'.
# Remove 'c' using --force-removal-of-dependent-packages
# Make sure nothing is removed.

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package='a', Depends='b', Essential='yes')
o.add(Package='b', Depends='c')
o.add(Package='c')

o.write_opk()
o.write_list()
opkgcl.update()

opkgcl.install('a')
if not opkgcl.is_installed('a'):
        opk.fail("Package 'a' installed but does not report as installed.")
if not opkgcl.is_installed('b'):
        opk.fail("Package 'b' installed but does not report as installed.")
if not opkgcl.is_installed('c'):
        opk.fail("Package 'c' installed but does not report as installed.")

opkgcl.remove('c', '--force-removal-of-dependent-packages')
if not opkgcl.is_installed('b'):
        opk.xfail("[internalsolv] Package 'b' was removed, but Essential package 'a' depends on it")
if not opkgcl.is_installed('c'):
        opk.fail("Package 'c' was removed, which breaks Essential package 'a'")
if not opkgcl.is_installed('a'):
        opk.fail("Essential package 'a' was incorrectly removed")
