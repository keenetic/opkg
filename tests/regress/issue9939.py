#!/usr/bin/env python3
#
# Reporter: ovidiu.vancea@ni.com
#
# What steps will reproduce the problem?
########################################
#
# 1.- Create package 'a' (version 1.0), which Recommends 'b (= 1.0)'
# 2.- Create package 'b' (version 1.0)
# 3.- Install 'a' (b 1.0 will also be installed)
# 4.- Create package 'a' (version 2.0), which Recommends 'b (= 2.0)'
# 5.- Create package 'b' (version 2.0)
# 6.- Upgrade package 'a' (opkg upgrade a)
#
# What is the expected output? What do you see instead?
#######################################
#
# Both package 'a' and 'b' should be upgraded to version 2.0.
# Instead, only package 'a' is upgraded.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

o.add(Package='a', Recommends='b (= 1.0)', Version='1.0')
o.add(Package='b', Version='1.0')
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install('a')

o = opk.OpkGroup()

o.add(Package='a', Recommends='b (= 2.0)', Version='2.0')
o.add(Package='b', Version='2.0')
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade('a')

if not opkgcl.is_installed("a", "2.0"):
    opk.fail("Package 'a' failed to upgrade")

if not opkgcl.is_installed("b", "2.0"):
    opk.fail("Package 'b' failed to upgrade")
