#! /usr/bin/env python3
#
# Reporter: Damine Gotfroi <gotfroi.damien@gmail.com>
#
# What steps will reproduce the problem?
########################################
#
# 1. Create package 'a' (1.0), which DEPENDS on 'b'.
# 2. Create package 'b' (1.0), which DEPENDS on 'c' and Provides 'd'
# 3. Create package 'c' (1.0), which PROVIDES 'e'
# 4. Install 'a'
# 5. Create package 'a' (2.0), which DEPENDS on 'f'.
# 6. Create package 'f' (2.0), which DEPENDS on 'g' and Provides 'd'
# 7. Create package 'g' (2.0), which PROVIDES 'e'
# 8. Upgrade
#
# What is the expected output? What do you see instead?
########################################
# Packages 'a' (2.0), 'b' (1.0),  'c' (1.0), f (2.0) and g (2.0) should be installed.
#
# Instead, opkg only packages 'a' (2.0) and c (1.0) are installed
#

import os
import opk, cfg, opkgcl

opk.regress_init()

_, version = opkgcl.opkgcl("--version")
libsolv = True if 'libsolv' in version else False

o = opk.OpkGroup()
o.add(Package='a', Version='1.0', Depends='b')
o.add(Package='b', Version='1.0', Depends='c', Provides='d')
o.add(Package='c', Version='1.0', Provides='e')
o.write_opk()
o.write_list()
opkgcl.update()

opkgcl.install('a')

if not opkgcl.is_installed('a', '1.0'):
    opk.fail("Package 'a' (1.0)  installed but does not report as installed.")
if not opkgcl.is_installed('b', '1.0'):
    opk.fail("Package 'b (1.0)' should be installed as a dependency of 'a' but does not report as installed.")
if not opkgcl.is_installed('c', '1.0'):
    opk.fail("Package 'c' (1.0) or 'e' should be installed as a dependency of 'b (1.0)' but does not report as installed.")

o.add(Package='a', Version='2.0', Depends='f')
o.add(Package='f', Version='2.0', Depends='g', Provides='d')
o.add(Package='g', Version='2.0', Provides='e')
o.write_opk()
o.write_list()
opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed('a', '2.0'):
    opk.fail("Package 'a' (2.0)  installed but does not report as installed.")
# The internalsolver is more agressive removing orphans than libsolv. For the moment, that's by design and sounds
# dangerous to modify the behavior
if not opkgcl.is_installed('b', '1.0') and libsolv:
    opk.fail("Package 'b (1.0)' should be installed as a dependency of 'a' but does not report as installed.")
if not opkgcl.is_installed('c', '1.0'):
    opk.fail("Package 'c' (1.0) should be installed as a dependency of 'b (1.0)' but does not report as installed.")
if not opkgcl.is_installed('f', '2.0'):
    opk.fail("Package 'f' (2.0) should be installed as a dependency of 'a (2.0)' but does not report as installed.")
if not opkgcl.is_installed('g', '2.0'):
    opk.fail("Package 'g' (2.0) should be installed as a dependency of 'f (2.0)' but does not report as installed.")
