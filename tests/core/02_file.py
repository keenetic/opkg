#! /usr/bin/env python3
#
# Install a package containing a file and check that the file is created. Remove
# the package and check that the file is removed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

# Create data file
f = open("test_file", "w")
f.write("Test")
f.close()

# Create package including test file
o = opk.OpkGroup()
pkg = opk.Opk(Package="a")
pkg.write(data_files=["test_file"])
o.addOpk(pkg)

o.write_list()

opkgcl.update()

# Delete data file used to create opk as it will be re-created when the package
# is installed
os.unlink("test_file")

# Test...
opkgcl.install("a")
if not opkgcl.is_installed("a"):
	opk.fail("Package 'a' installed but reports as not installed.")
if not os.path.exists("%s/test_file" % cfg.offline_root):
	opk.fail("Package 'a' installed but file 'test_file' not created.")

opkgcl.remove("a")
if opkgcl.is_installed("a"):
	opk.fail("Package 'a' removed but still reports as being installed.")
if os.path.exists("%s/test_file" % cfg.offline_root):
	opk.fail("Package 'a' removed but file 'test_file' still present.")
