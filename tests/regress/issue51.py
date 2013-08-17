#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

open("foo", "w").close()
a1 = opk.Opk(Package="a")
a1.write(data_files=["foo"])
os.rename("a_1.0_all.opk", "a_with_foo.opk")

opkgcl.install("a_with_foo.opk")

# ----
opkgcl.install("a_with_foo.opk")

open("bar", "w").close()
o = opk.OpkGroup()
a2 = opk.Opk(Package="a")
a2.write(data_files=["foo", "bar"])
o.opk_list.append(a2)
o.write_list()

os.unlink("foo")
os.unlink("bar")

opkgcl.update()
opkgcl.install("a", "--force-reinstall")

foo_fullpath = "{}/foo".format(cfg.offline_root)
bar_fullpath = "{}/bar".format(cfg.offline_root)

if not os.path.exists(foo_fullpath) or not os.path.exists(bar_fullpath):
	opk.fail("Files foo and/or bar are missing.")

a_files = opkgcl.files("a")
if not foo_fullpath in a_files or not bar_fullpath in a_files:
	opk.fail("Package 'a' does not own foo and/or bar.")

opkgcl.remove("a")

if os.path.exists(foo_fullpath) or os.path.exists(bar_fullpath):
	opk.fail("Files foo and/or bar still exist "
				"after removal of package 'a'.")

# ----
o = opk.OpkGroup()
a2 = opk.Opk(Package="a")
a2.write()
o.opk_list.append(a2)
o.write_list()


opkgcl.update()

opkgcl.install("a", "--force-reinstall")

if os.path.exists(foo_fullpath):
	opk.fail("File 'foo' not orphaned as it should be.")

if foo_fullpath in opkgcl.files("a"):
	opk.fail("Package 'a' incorrectly owns file 'foo'.")

opkgcl.remove("a")
