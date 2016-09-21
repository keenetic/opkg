#!/usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.12A")
o.add(Package="a", Version="1.12B")
o.add(Package="b", Version="1.2")
o.add(Package="b", Version="1.12")
o.add(Package="c", Version="001.1535A")
o.add(Package="c", Version="001.CIBUILDX20160919_153521")
o.add(Package="d", Version="1:1.12B")
o.add(Package="d", Version="2:1.12A")
o.add(Package="e", Version="2.0")
o.add(Package="e", Version="1:1.0")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if opkgcl.is_installed("a", "1.12A"):
    opk.fail("Wrong 'a' package version installed")
elif not opkgcl.is_installed("a", "1.12B"):
    opk.fail("No 'a' package installed")

opkgcl.install("b")
if opkgcl.is_installed("b", "1.2"):
    opk.fail("Wrong 'b' package version installed")
elif not opkgcl.is_installed("b", "1.12"):
    opk.fail("No 'b' package installed")

opkgcl.install("c")
if opkgcl.is_installed("c", "001.1535A"):
    opk.fail("Wrong 'c' package version installed")
elif not opkgcl.is_installed("c", "001.CIBUILDX20160919_153521"):
    opk.fail("No 'c' package installed")

opkgcl.install("d")
if opkgcl.is_installed("d", "1:1.12B"):
    opk.fail("Wrong 'd' package version installed")
elif not opkgcl.is_installed("d", "2:1.12A"):
    opk.fail("No 'd' package installed")

opkgcl.install("e")
if opkgcl.is_installed("e", "2.0"):
    opk.fail("Wrong 'e' package version installed")
elif not opkgcl.is_installed("e", "1:1.0"):
    opk.fail("No 'e' package installed")
