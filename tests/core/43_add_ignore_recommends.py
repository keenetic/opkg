#! /usr/bin/env python3
#
# Create package 'a' (1.0) which Recommends 'c'.
# Install 'a' with --add-ignore-recommends 'c'.
# Check that only 'a' (1.0) is installed.
# Create package 'b' which Depends on 'c'.
# Install 'a' & 'b', with --add-ignore-recommends 'c'.
# Verify that 'a','b' & 'c' are installed.
# Uninstall 'b' & 'c'.
# Create package 'a' (2.0), which Recommends 'c'.
# Upgrade 'a' with --add-ignore-recommends 'c'
# Verify that only 'a' (2.0) is installed
#

import os
import opk, cfg, opkgcl

opk.regress_init()
o = opk.OpkGroup()

o.add(Package='a', Recommends='c', Version='1.0')
o.add(Package='b', Depends='c')
o.add(Package='c')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install('a', '--add-ignore-recommends c')

if not opkgcl.is_installed('a'):
	opk.fail("Package 'a' installed but reports as not installed.")

if opkgcl.is_installed('c'):
	opk.xfail("[libsolv<0.7.3] Package 'c' should not have been installed since it was in --add-ignore-recommends.")

opkgcl.remove('a')
opkgcl.install('a b', '--add-ignore-recommends c')

if not opkgcl.is_installed('a'):
	opk.fail("Package 'a' installed but reports as not installed.")

if not opkgcl.is_installed('b'):
	opk.fail("Package 'b' installed but reports as not installed.")

if not opkgcl.is_installed('c'):
	opk.fail("Package 'c' should have been installed since 'b' depends on it.")

opkgcl.remove('b c', '--force-depends')
o.add(Package='a', Recommends='c', Version='2.0')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.upgrade('a', '--add-ignore-recommends c')

if not opkgcl.is_installed('a', '2.0'):
	opk.fail("Package 'a (2.0)' installed but reports as not installed.")

if opkgcl.is_installed('c'):
	opk.fail("Package 'c' should not have been installed since it was in --add-ignore-recommends.")
