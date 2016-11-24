#! /usr/bin/env python3
#
# Reporter: alejandro.delcastillo@ni.com
#
# Test the use of globs during install and
# remove operations
#

import os
import opk, cfg, opkgcl
import re

opk.regress_init()

o = opk.OpkGroup()
o.add(Package='a-b', Version='1.0')
o.add(Package='a-c', Version='1.0')
o.add(Package='b-a', Version='1.0')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("'a*'")
if not opkgcl.is_installed('a-b'):
    opk.xfail("[internalsolv] Package 'a-b' not installed but reports as being installed.")
if not opkgcl.is_installed('a-c'):
    opk.fail("Package 'a-c' not installed but reports as being installed.")
if opkgcl.is_installed('b-a'):
    opk.fail("Package 'b-a' was incorrectly installed")

opkgcl.remove("'a*'")
if opkgcl.is_installed('a-b'):
    opk.fail("Package 'a-b' failed to remove")
if opkgcl.is_installed('a-c'):
    opk.fail("Package 'a-c' failed to remove")

opkgcl.install("'?-b'")
if not opkgcl.is_installed('a-b'):
    opk.fail("Package 'a-b' not installed but reports as being installed.")
if opkgcl.is_installed('a-c'):
    opk.fail("Package 'a-c' was incorrectly installed")
if opkgcl.is_installed('b-a'):
    opk.fail("Package 'b-a' was incorrectly installed")

opkgcl.remove("'?-b'")
if opkgcl.is_installed('a-b'):
    opk.fail("Package 'a-b' failed to remove")

opkgcl.install("a-[bc]")
if not opkgcl.is_installed('a-b'):
    opk.fail("Package 'a-b' not installed but reports as being installed.")
if not opkgcl.is_installed('a-c'):
    opk.fail("Package 'a-c' not installed but reports as being installed.")
if opkgcl.is_installed('b-a'):
    opk.fail("Package 'b-a' was incorrectly installed")

o.add(Package='a-b', Version='2.0')
o.add(Package='a-c', Version='2.0')
o.add(Package='b-a', Version='2.0')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.upgrade("'a*'")
if not opkgcl.is_installed('a-b', '2.0'):
    opk.fail("New version of package 'a-b' available during upgrade but was not installed.")
if not opkgcl.is_installed('a-c', '2.0'):
    opk.fail("New version of package 'a-c' available during upgrade but was not installed.")
if opkgcl.is_installed('b-a'):
    opk.fail("Package 'b-a' was incorrectly installed")

opkgcl.remove("'a*'")
opkgcl.install('b-a=1.0')

opkgcl.upgrade("'?-a'")
if not opkgcl.is_installed('b-a', '2.0'):
    opk.fail("New version of package 'b-a' available during upgrade but was not installed.")
if opkgcl.is_installed('a-b'):
    opk.fail("Package 'a-b' was incorrectly installed")
if opkgcl.is_installed('a-c'):
    opk.fail("Package 'a-c' was incorrectly installed")

opkgcl.remove("'*'")
opkgcl.install('a-b=1.0')
opkgcl.install('a-c=1.0')

opkgcl.upgrade("a-[bc]")
if not opkgcl.is_installed('a-b', '2.0'):
    opk.fail("New version of package 'a-b' available during upgrade but was not installed..")
if not opkgcl.is_installed('a-c', '2.0'):
    opk.fail("New version of package 'a-c' available during upgrade but was not installed..")
if opkgcl.is_installed('b-a'):
    opk.fail("Package 'b-a' was incorrectly installed")
