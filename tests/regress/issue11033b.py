#!/usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

# Package A creates a symlink
os.mkdir('lib64')
os.symlink('lib64', 'lib')
pkg = opk.Opk(Package='a')
pkg.write(data_files=['lib', 'lib64'])
o.addOpk(pkg)

# Package B creates the same symlink
pkg = opk.Opk(Package='b')
pkg.write(data_files=['lib', 'lib64'])
o.addOpk(pkg)
os.remove('lib')
os.rmdir('lib64')

# Package C creates a different symlink
os.mkdir('lib32')
os.symlink('lib32', 'lib')
pkg = opk.Opk(Package='c')
pkg.write(data_files=['lib', 'lib32'])
o.addOpk(pkg)
os.remove('lib')
os.rmdir('lib32')

o.write_list()

opkgcl.update()

opkgcl.install('a')
opkgcl.install('b')
if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' could not be installed")

opkgcl.install('c')
if opkgcl.is_installed('c'):
    opk.fail("Package 'c' installed despite symlink conflict")
