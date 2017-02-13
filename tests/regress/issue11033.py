#!/usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

# Package A creates a symlink
os.mkdir('ro')
os.symlink('../rw/config', 'ro/config')
pkg = opk.Opk(Package='a')
pkg.write(data_files=['ro'])
o.addOpk(pkg)
os.remove('ro/config')

# Package B puts files in the directory
os.mkdir('ro/config')
with open('ro/config/b.conf', 'w') as f:
    f.write('Test')
pkg = opk.Opk(Package='b')
pkg.write(data_files=['ro'])
o.addOpk(pkg)
os.remove('ro/config/b.conf')
os.rmdir('ro/config')
os.rmdir('ro')

o.write_list()

opkgcl.update()

opkgcl.install('a')
if not os.path.islink("%s/ro/config" % cfg.offline_root):
    opk.fail("Package 'a' didn't install required file")
opkgcl.install('b')
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' installed despite symlink conflict")
opkgcl.remove('b')
opkgcl.remove('a')

opkgcl.install('b')
if not os.path.exists("%s/ro/config/b.conf" % cfg.offline_root):
    opk.fail("Package 'b' didn't install required file")
opkgcl.install('a')
if opkgcl.is_installed('a'):
    opk.fail("Package 'a' installed despite symlink conflict")
