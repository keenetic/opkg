#! /usr/bin/env python3
#
# Verifies intercept scripts can be disabled by an opkg conf token.
#
# Installs a package with preinst, postinst, prerm, and postrm scripts
# calling `ls` in sysroot with an ls intercept. The intercepted ls
# script logs a message to indicate when it's called. The test sequence
# runs with intercepts_dir unset and intercepts_dir=/dev/null to verify
# ls is only intercepted in the former case and not the latter.
#

import os, tempfile, errno
import opk, cfg, opkgcl

opk.regress_init()

def readFile(path):
    with open(path, 'r') as f:
        return f.read()

def truncFile(path):
    with open(path, 'w') as f:
        pass

def appendFile(path, string):
    with open(path, 'a') as f:
        f.write(string)

def touch_dir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

# Log file appended to by the intercept script.
# Test sequence (below) removes this file before installing packages,
# then compares it's contents against an expected value.
TEST_LOG = os.path.join(cfg.offline_root, "intercepts_test.log")

def prepare_sysroot():
    '''
    Creates intercept script and test package.
    '''
    testprefix=cfg.offline_root+os.environ['DATADIR']
    # Add a intercept script for ls to the sysroot
    touch_dir('%s/opkg/intercept' % testprefix)
    with open('%s/opkg/intercept/ls' % testprefix, 'w') as f:
        os.fchmod(f.fileno(), 0o755)
        f.write('\n'.join([
            '#!/bin/sh',
            'set -e',
            'exeFileName="`basename "$1"`"',
            'echo -n \"intercept from $exeFileName:\" >> \'%s\'' % TEST_LOG,
            'echo "ls $* was intercepted"',
        ]))

    o = opk.OpkGroup()

    # Add a test package
    a = opk.Opk(Package='a')
    a.postinst = '\n'.join([
        '#!/bin/sh',
        'set -e',
        'ls "$0"',
    ])
    o.addOpk(a)

    o.write_opk()
    o.write_list()

prepare_sysroot()
opkgcl.update()

# First install with intercepts enabled
truncFile(TEST_LOG)
if opkgcl.install('a') != 0:
    opk.fail('Failed to install test package')
if opkgcl.remove('a') != 0:
    opk.fail('Failed to remove test package')
if readFile(TEST_LOG) != 'intercept from a.postinst:':
    opk.fail('Unexpected intercept log')

sysconfdir=os.environ['SYSCONFDIR']
testconfdir=cfg.offline_root+sysconfdir
# Reconfigure opkg to disable intercepts
appendFile('%s/opkg/opkg.conf' % testconfdir, 'option intercepts_dir /dev/null\n')

# Re-run the test
truncFile(TEST_LOG)
if opkgcl.install('a') != 0:
    opk.fail('Failed to install test package')
if opkgcl.remove('a') != 0:
    opk.fail('Failed to remove test package')
if readFile(TEST_LOG) != '':
    opk.fail('Unexpected intercept log')
