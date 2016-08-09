#! /usr/bin/env python3
#
# Reported by boris.bischoff, Aug 17, 2012
#
# What steps will reproduce the problem?
#   1. Install a package with "opkg install" that contains symlinks to
#      directories which will be created.
#   2. Remove the very same package with "opkg remove".
#
# What is the expected output? What do you see instead?
#   I expect all files/directories to be deleted, even the symlinks.
#   Instead, all files/directories are deleted except for the symlinks.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

os.mkdir("test_dir")
os.symlink("test_dir", "test_link")
pkg = opk.Opk(Package="a", Version="1.0")
pkg.write(data_files=["test_link", "test_dir"])
o.addOpk(pkg)
o.write_list()
os.unlink("test_link")
os.rmdir("test_dir")

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to install.")
if not os.path.exists("%s/test_dir" % cfg.offline_root):
	opk.fail("Package 'a' installed but 'test_dir' not created")
if not os.path.lexists("%s/test_link" % cfg.offline_root):
	opk.fail("Package 'a' installed but 'test_link' not created")

opkgcl.remove("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' failed to remove.")
if os.path.exists("%s/test_dir" % cfg.offline_root):
	opk.fail("Package 'a' removed but 'test_dir' not deleted")
if os.path.lexists("%s/test_link" % cfg.offline_root):
	opk.fail("Package 'a' removed but 'test_link' not deleted")
