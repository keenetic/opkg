#! /usr/bin/env python3
#
# Install package 'a' with a Depends on 'b'. Make available a new version of
# 'a' that Conflicts with 'b'. Dist-upgrade and make sure 'a' is upgraded to
# the new version, while 'b' is removed.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

o.add(Package='a', Version="1.0", Depends='b')
o.add(Package='b')
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install('a')

if not opkgcl.is_installed('a'):
    opk.fail("Package 'a' installed but reports as not installed.")
if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' installed but reports as not installed.")

o.add(Package="a", Version="2.0", Conflicts='b')
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.distupgrade()

if not opkgcl.is_installed('a', '2.0'):
    opk.xfail("[internalsolv] New version of package 'a' available during dist-upgrade but was not installed")

if opkgcl.is_installed('b'):
    opk.fail("Package 'b' was not removed.")
