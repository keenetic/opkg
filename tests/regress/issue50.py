#! /usr/bin/env python3
#
# Reporter: Graham Gower
#
# What steps will reproduce the problem?
# ======================================
#
# 1. Create package a version 1, containing file /foo.
# 2. Create package a version 2, without file /foo, depending on package b.
# 3. Create package b version 1, containing file /foo which Conflicts  a (<< 2.0)
# 4. Create package repository containing a_2 and b_1.
# 5. Install a_1 manually.
# 6. opkg update; opkg upgrade
#
#
# What is the expected output? What do you see instead?
# =====================================================
#
# File /foo is expected to exist and be provided by package b_1. This is not the
# case, file /foo has disappeared.
#
# opkg_remove.c: remove_data_files_and_list() should double check that the file
# belongs to the package before removing it.
#
#
# Status
# ======
#
# Fixed by r531-r534,r536

import os
import opk, cfg, opkgcl

opk.regress_init()

open("foo", "w").close()
a1 = opk.Opk(Package="a", Version="1.0")
a1.write(data_files=["foo"])

opkgcl.install("a_1.0_all.opk")

o = opk.OpkGroup()
a2 = opk.Opk(Package="a", Version="2.0", Depends="b")
a2.write()
b1 = opk.Opk(Package="b", Version="1.0", Conflicts="a (<< 2.0)", Replaces="a (<< 2.0)")
b1.write(data_files=["foo"])
o.opk_list.append(a2)
o.opk_list.append(b1)
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

opkgcl.remove("a")
opkgcl.remove("b")
