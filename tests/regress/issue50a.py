#! /usr/bin/env python3
#
# Verifies that a package split into 3 subpackages can be upgraded correctly
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create package a version 1, containing files /foo and /foo1.
# 2. Create package a version 2, without files /foo and /foo1, depending on packages a & b
# 3. Create package b version 1, containing file /foo which Conflicts/Replaces  a (<< 2.0)
# 4. Create package c version 1, containing file /foo1 which Conflicts/Replaces  a (<< 2.0)
# 5. Create package repository containing a_2,  b_1 and c_1.
# 6. Install a_1 manually.
# 7. opkg update; opkg upgrade
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# Files /foo & /foo1 are expected to exist and be provided by packages b and c.
#
#

import os
import opk, cfg, opkgcl

opk.regress_init()

open("foo", "w").close()
open("foo1", "w").close()
a1 = opk.Opk(Package="a", Version="1.0")
a1.write(data_files=["foo", "foo1"])

opkgcl.install("a_1.0_all.opk")

o = opk.OpkGroup()
a2 = opk.Opk(Package="a", Version="2.0", Depends="b,c")
a2.write()
b1 = opk.Opk(Package="b", Version="1.0", Conflicts="a (<< 2.0)", Replaces="a (<< 2.0)")
b1.write(data_files=["foo"])
c1 = opk.Opk(Package="c", Version="1.0", Conflicts="a (<< 2.0)", Replaces="a (<< 2.0)")
c1.write(data_files=["foo1"])
o.opk_list.append(a2)
o.opk_list.append(b1)
o.opk_list.append(c1)
o.write_list()

os.unlink("foo")

opkgcl.update()
status = opkgcl.upgrade("a")

if status != 0:
        opk.fail("Upgrade operation failed (Return value was different than 0)")

if not opkgcl.is_installed("a", "2.0"):
	opk.fail("Package 'a_2.0' not installed.")

foo_fullpath = "{}/foo".format(cfg.offline_root)

if not os.path.exists(foo_fullpath):
	opk.fail("File 'foo' incorrectly orphaned.")

if not foo_fullpath in opkgcl.files("b"):
	opk.fail("Package 'b' does not own file 'foo'.")

foo1_fullpath = "{}/foo1".format(cfg.offline_root)

if not os.path.exists(foo1_fullpath):
	opk.fail("File 'foo1' incorrectly orphaned.")

if not foo1_fullpath in opkgcl.files("c"):
	opk.fail("Package 'c' does not own file 'foo'.")


opkgcl.remove("a")
opkgcl.remove("b")
opkgcl.remove("c")
