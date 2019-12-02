#! /usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

# Package 'a' creates a dir, and a symlink to that dir
os.mkdir('lib64')
os.symlink('lib64', 'lib')
open('lib64/testfile.txt', 'w').close()
a = opk.Opk(Package='a')
a.write(data_files=['lib', 'lib64', 'lib64/testfile.txt'])
o.addOpk(a)
os.remove('lib64/testfile.txt')
os.remove('lib')
os.rmdir('lib64')

# Package 'b' creates a file in a dir that follows the symlink created by 'a'
os.mkdir('lib')
open('lib/testfile2.txt', 'w').close()
b = opk.Opk(Package='b')
b.write(data_files=['lib', 'lib/testfile2.txt'])
o.addOpk(b)
os.remove('lib/testfile2.txt')
os.rmdir('lib')

o.write_list()
opkgcl.update()

opkgcl.install('a')
if not os.path.exists("%s/lib64/testfile.txt" % cfg.offline_root):
    opk.fail("Package 'a' didn't install required file")

opkgcl.install('b')
if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' could not be installed")

if not os.path.exists("%s/lib64/testfile2.txt" % cfg.offline_root):
    opk.fail("Package 'b' didn't install required file")

opkgcl.remove('a')
if not os.path.exists("%s/lib64/testfile2.txt" % cfg.offline_root):
    opk.fail("Package 'a' removed package 'b' file")

if not os.path.exists("%s/lib" % cfg.offline_root):
    opk.fail("Package 'a' incorrectly removed symlink that should belong to Package 'b'")

opkgcl.remove('b')
if os.path.exists("%s/lib64/testfile2.txt" % cfg.offline_root):
    opk.fail("Package 'b' removed but file 'testfile2.txt' still present.")
