#! /usr/bin/env python3
#
# Create package 'a(1.0)' with short description only.
# Create package 'b(1.0)' with a complete multiline description.
# Check that info command for 'a' always includes the full description.
# Check that info command for 'b' depends on the short-description flag.
#

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Description="Just Short")
o.add(Package="b", Version="1.0", Description="Terse\n Verbose description\n across multiple lines")
o.write_opk()
o.write_list()

opkgcl.update()

info = opkgcl.info("a")
if "Just Short" not in info:
        opk.fail("Package 'a' description should contain 'Just Short'")

info = opkgcl.info("a", flags="--short-description")
if "Just Short" not in info:
        opk.fail("Package 'a' description should contain 'Just Short'")

info = opkgcl.info("b")
if "Terse" not in info:
        opk.fail("Package 'b' description should contain 'Terse'")
if "Verbose description\n across multiple lines" not in info:
        opk.fail("Package 'b' description should contain all lines because short-description was not specified")

info = opkgcl.info("b", flags="--short-description")
if "Terse" not in info:
        opk.fail("Package 'b' description should contain 'Terse'")
if "Verbose description\n across multiple lines" in info:
        opk.fail("Package 'b' description should contain only the first line because short-description was specified")
