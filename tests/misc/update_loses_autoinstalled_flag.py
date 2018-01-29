#! /usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

_, version = opkgcl.opkgcl("--version")
libsolv = True if 'libsolv' in version else False

open("asdf", "w").close()
a = opk.Opk(Package="a", Version="1.0", Depends="b")
a.write()
b = opk.Opk(Package="b", Version="1.0")
b.write(data_files=["asdf"])

o = opk.OpkGroup()
o.addOpk(a)
o.addOpk(b)
o.write_list()

opkgcl.update()
opkgcl.install("a")

if not opkgcl.is_autoinstalled("b"):
	opk.fail("b is not autoinstalled")

a = opk.Opk(Package="a", Version="2.0", Depends="b")
a.write()
b = opk.Opk(Package="b", Version="2.0")
b.write(data_files=["asdf"])

o = opk.OpkGroup()
o.addOpk(a)
o.addOpk(b)
o.write_list()

opkgcl.update()
opkgcl.upgrade();

if not opkgcl.is_autoinstalled("b"):
	opk.fail("b is not autoinstalled anymore")

# The internalsolver is more agressive removing orphans than libsolv. On libsolv upgrades, no
# packages are removed (on dist-upgrade, removal is allowed)
if not libsolv:
    a = opk.Opk(Package="a", Version="3.0")
    a.write(data_files=["asdf"])
    os.unlink("asdf")

    o = opk.OpkGroup()
    o.addOpk(a)
    o.write_list()

    opkgcl.update()
    opkgcl.upgrade();

    if opkgcl.is_installed("b", "2.0"):
        opk.fail("b is still installed")

    if not opkgcl.is_installed("a", "3.0"):
        opk.fail("a is not installed")
