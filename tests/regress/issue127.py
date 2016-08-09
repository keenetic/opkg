#! /usr/bin/env python3
#
# Issue 127 reported by muusclaus
#
# 1. call "opkg upgrade PACKAGE_NAME"
# 2. PACKAGE_NAME is a not installed package
#
# What is the expected output?
# I expected to get an error message, that the package is not installed yet.
#
# What do you see instead?
# The package will be installed like as I do an "opkg install ..."
#

import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.upgrade("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed by upgrade.")
