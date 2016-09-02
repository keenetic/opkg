#! /usr/bin/env python3
#
# Test that --download-only flag doesn't change the state
# of the system during install/remove/upgrade operations
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Depends="b")
o.add(Package="b")
o.write_opk()
o.write_list()
opkgcl.update()
opkgcl.install("a")

o = opk.OpkGroup()
o.add(Package="a", Version="2.0", Depends="c")
o.add(Package="c")
o.write_opk()
o.write_list()

opkgcl.update()

status = opkgcl.install("c", "--download-only")
if opkgcl.is_installed("c"):
    opk.fail("Pacakge c was installed during a --download-only operation")

status = opkgcl.remove("a", "--download-only")
if not opkgcl.is_installed("a"):
    opk.fail("Package a was removed during a --download-only operation")

status = opkgcl.upgrade("--download-only")
if not opkgcl.is_installed("a", "1.0"):
    opk.fail("Pacakge a was upgraded during a --download-only operation")
if not opkgcl.is_installed("b"):
    opk.fail("Package b was incorrectly removed during a --download-only operation")
if opkgcl.is_installed("c"):
    opk.fail("Package c was incorrectly installed during a --download-only operation")
