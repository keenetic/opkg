#!/usr/bin/env python3

import os
import opk, cfg, opkgcl
import re
vardir=os.environ['VARDIR']

re_half_installed = re.compile('Status: \w+ \w+ half-installed')

def is_half_installed(pkg_name):
    status_path = ("{}"+vardir+"/lib/opkg/status").format(cfg.offline_root)
    if not os.path.exists(status_path):
        return False
    with open(status_path, "r") as status_file:
        status = status_file.read()
    index_start = status.find("Package: {}".format(pkg_name))
    while index_start >= 0:
        index_end = status.find("\n\n", index_start)
        match = re_half_installed.search(status[index_start:index_end])
        if match is not None:
            return True
        index_start = status.find("Package: {}".format(pkg_name), index_end)
    return False

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

opkgcl.remove('b')
if opkgcl.is_installed('b'):
    opk.fail("Package 'b' failed to remove")
if is_half_installed('b'):
    opk.fail("Failed to clean up package 'b' from status file")

b2 = opk.Opk(Package='b', Version='2.0')
b2.write()

o.opk_list.append(b2)
o.write_list()

opkgcl.update()

opkgcl.install('b')
if not opkgcl.is_installed('b', '2.0'):
    opk.fail("Package 'b' failed to install")
if is_half_installed('b'):
    opk.fail("Failed to clean up previous package 'b' from status file")
