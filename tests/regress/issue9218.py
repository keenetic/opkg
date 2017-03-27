#! /usr/bin/env python3
#
# Reporter: alejandro.delcastillo@ni.com
#
# What steps will reproduce the problem?
# ======================================
#
# 1.- Create package 'a', which DEPENDS on 'b'
# 2.- Create package 'b', which DEPENDS on 'c'
# 3.- Create package 'c'
# 4.- Install 'a'
#
# What is the expected output? What do you see instead?
# =====================================================
#
# The order of installation should be c -> b -> a,
# instead it is b -> c -> a.
#

import os
import opk, cfg, opkgcl

opk.regress_init()
o = opk.OpkGroup()

a = opk.Opk(Package='a', Depends='b')
a.write()

# Package 'b' has a preinst script that relies
# on a file installed by package 'c'
b = opk.Opk(Package='b', Depends='c')
b.preinst = '\n'.join([
    '#!/bin/sh',
    'set e',
    'ls %s/test_file' % cfg.offline_root,
])
b.write()

f = open('test_file', 'w')
f.write('Test')
f.close()
c = opk.Opk(Package='c')
c.write(data_files=['test_file'])

o.opk_list.append(a)
o.opk_list.append(b)
o.opk_list.append(c)
o.write_list()

opkgcl.update()

os.unlink('test_file')

if opkgcl.install('a') != 0:
    opk.fail("Installation of Pacakge 'a' failed (Return value was different than 0)")
if not opkgcl.is_installed("a"):
    opk.fail("Package 'a' installed but reports as not installed.")
if not opkgcl.is_installed("b"):
    opk.fail("Package 'b' installed but reports as not installed.")
if not opkgcl.is_installed("c"):
    opk.fail("Package 'c' installed but reports as not installed.")
