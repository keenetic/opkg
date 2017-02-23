#!/usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

# Package A creates a symlink
os.mkdir('lib64')
os.symlink('lib64', 'lib')
with open('static_file.txt', 'w') as f:
    f.write('Test')
pkg = opk.Opk(Package='a')
pkg.write(data_files=['lib', 'lib64', 'static_file.txt'])
o.addOpk(pkg)
os.remove('static_file.txt')

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

# Package D creates a symlink to a file (but doesn't include the file)
os.symlink('static_file.txt', 'link.txt')
pkg = opk.Opk(Package='d')
pkg.write(data_files=['link.txt'])
o.addOpk(pkg)

# Package E creates the same file symlink
pkg = opk.Opk(Package='e')
pkg.write(data_files=['link.txt'])
o.addOpk(pkg)
os.remove('link.txt')

o.write_list()

opkgcl.update()

opkgcl.install('a')
opkgcl.install('b')
if not opkgcl.is_installed('b'):
    opk.fail("Package 'b' could not be installed")

opkgcl.install('c')
if opkgcl.is_installed('c'):
    opk.fail("Package 'c' installed despite symlink conflict")

opkgcl.install('d')
opkgcl.install('e')
if opkgcl.is_installed('e'):
    opk.fail("Package 'e' installed despite symlink conflict")
