#!/usr/bin/env python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()

with open('conflict_file', 'w') as f:
    f.write('Test')
a = opk.Opk(Package='a', Version='1.0')
a.write(data_files=['conflict_file'])
b = opk.Opk(Package='b', Version='1.0')
b.write(data_files=['conflict_file'])

o.opk_list.append(a)
o.opk_list.append(b)
o.write_list()

opkgcl.update()

opkgcl.install('a')
opkgcl.install('b')
if not opkgcl.is_installed('a', '1.0'):
    opk.fail("Package 'a' failed to install")
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' installed despite file conflict")

b2 = opk.Opk(Package='b', Version='2.0')
b2.write(data_files=['conflict_file'])

o.opk_list.append(b2)
o.write_list()

opkgcl.update()

# Installing another version to ensure the state of the first gets updated
# even though the second also contains a conflict
opkgcl.install('b_2.0_all.opk')
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' installed despite file conflict")

b3 = opk.Opk(Package='b', Version='3.0')
b3.write()

o.opk_list.append(b3)
o.write_list()

opkgcl.update()

# This should install a working version, removing the debris from the failed
# install attempts above
opkgcl.install('b_3.0_all.opk')
if not opkgcl.is_installed('b', '3.0'):
    opk.fail("Package 'b' failed to install")
