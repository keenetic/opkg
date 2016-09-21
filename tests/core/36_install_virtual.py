#! /usr/bin/env python3
#
# Test opkg when specifying a virtual package to install:
#       opkg install <virtual_pkg_name>
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version='1.0', Provides="virt")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("virt")
if not opkgcl.is_installed("a", "1.0"):
    opk.fail("Package 'virt' (provided by 'a') failed to install.")

o.add(Package="a", Version='2.0', Provides="virt")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.upgrade("virt")
if not opkgcl.is_installed("a", "2.0"):
    opk.fail("Package 'virt' (provided by 'a') failed to upgrade.")
