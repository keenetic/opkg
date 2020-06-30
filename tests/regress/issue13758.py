#! /usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

o.add(Package='a', Recommends='b, c (> 1.0), d')
o.add(Package='b')
o.add(Package='c', Version='2.0' )
o.add(Package='d')
o.add(Package='e')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install('a', '--add-ignore-recommends b')

if not opkgcl.is_installed('a'):
    opk.fail("Package 'a' installed but reports as not installed.")

if not opkgcl.is_installed('c'):
    opk.fail("Package 'c' installed but reports as not installed.")

if not opkgcl.is_installed('d'):
    opk.fail("Package 'd' installed but reports as not installed.")

if opkgcl.is_installed('b'):
    opk.xfail("[libsolv<0.7.3] Package 'b' should not have been installed since it was in --add-ignore-recommends.")

opkgcl.install('e')

if not opkgcl.is_installed('e'):
    opk.fail("Package 'e' installed but reports as not installed.")
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' should not have been installed since 'c' doesn't depends/recommends it.")

opkgcl.remove("'*'", "--force-depends")

opkgcl.install('a', '--add-ignore-recommends c')

if not opkgcl.is_installed('a'):
    opk.fail("Package 'a' installed but reports as not installed.")

if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' installed but reports as not installed.")

if not opkgcl.is_installed('d'):
    opk.fail("Package 'd' installed but reports as not installed.")

if opkgcl.is_installed('c'):
    opk.xfail("Package 'c' should not have been installed since it was in --add-ignore-recommends.")

opkgcl.install('e')

if not opkgcl.is_installed('e'):
    opk.fail("Package 'e' installed but reports as not installed.")
if opkgcl.is_installed('c'):
    opk.fail("Package 'c' should not have been installed since 'c' doesn't depends/recommends it.")