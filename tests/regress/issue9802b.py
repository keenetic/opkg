#! /usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Depends="b")
o.add(Package="b", Version="1.0")
o.add(Package="b", Version="2.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("b_2.0_all.opk")
if not opkgcl.is_installed("b", "2.0"):
    opk.fail("Package 'b' failed to install")

opkgcl.install("a")
if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' failed to install")
if not opkgcl.is_installed("b", "2.0"):
    opk.fail("Package 'b' downgraded but downgrade was not necessary")
