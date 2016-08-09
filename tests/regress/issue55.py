#! /usr/bin/env python3
#
# Report and fix from "paradox.kahn" in a google code comment to r543.
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create a package containing a symlink to a file with a long file name
# 	(longer than 100 characters)
# 2. Install the package
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# The symlink should be created and point to the appropriate file. Instead, the
# symlink is not created at all.
#
#
# Status
# ======
#
# Fixed in r544.

import os
import opk, cfg, opkgcl

opk.regress_init()

long_filename = 110*"a"

os.symlink(long_filename, "linky")
a = opk.Opk(Package="a")
a.write(data_files=["linky"])
os.unlink("linky")
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' not installed.")

if not os.path.lexists("{}/linky".format(cfg.offline_root)):
	opk.fail("symlink to file with a name longer than 100 "
					"characters not created.")

opkgcl.remove("a")
