#! /usr/bin/env python3
#
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

# Create a 1.0 containing file "test1" and install by name
f = open("test1", "w")
f.write("Test")
f.close()
pkg = opk.Opk(Package="a", Version="1.0")
pkg.write(data_files=["test1"])
o.addOpk(pkg)
o.write_list()
os.unlink("test1")

opkgcl.update()

opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")
if not os.path.exists("%s/test1" % cfg.offline_root):
	opk.fail("Package 'a' installed but file 'test1' not created.")

# Destroy the old package
os.unlink("a_1.0_all.opk")

# Create a 2.0 containing file "test2" and upgrade by URI
f = open("test2", "w")
f.write("Test")
f.close()
pkg = opk.Opk(Package="a", Version="1.0")
pkg.write(data_files=["test2"])
o.addOpk(pkg)
o.write_list()
os.unlink("test2")

opkgcl.install("a_1.0_all.opk", "--force-reinstall")

if not os.path.exists("%s/test2" % cfg.offline_root):
	opk.fail("Package 'a' force reinstalled but new file 'test2' not created.")
if os.path.exists("%s/test1" % cfg.offline_root):
	opk.fail("Package 'a' force reinstalled but old file 'test1' not removed.")
