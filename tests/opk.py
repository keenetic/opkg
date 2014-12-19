import tarfile, os, sys
import cfg
import errno

__appname = sys.argv[0]

class Opk:
	valid_control_fields = ["Package", "Version", "Depends", "Provides",\
			"Replaces", "Conflicts", "Suggests", "Recommends",\
			"Section", "Architecture", "Maintainer", "MD5Sum",\
			"Size", "InstalledSize", "Filename", "Source",\
			"Description", "OE", "Homepage", "Priority",\
			"Conffiles"]

	def __init__(self, **control):
		for k in control.keys():
			if k not in self.valid_control_fields:
				raise Exception("Invalid control field: "
						"{}".format(k))
		if "Package" not in control.keys():
			print("Cannot create opk without Package name.\n")
			return None
		if "Architecture" not in control.keys():
			control["Architecture"] = "all"
		if "Version" not in control.keys():
			control["Version"] = "1.0"
		self.control = control

	def write(self, tar_not_ar=False, data_files=None):
		filename = "{Package}_{Version}_{Architecture}.opk"\
						.format(**self.control)
		if os.path.exists(filename):
			os.unlink(filename)
		if os.path.exists("control"):
			os.unlink("control")
		if os.path.exists("control.tar.gz"):
			os.unlink("control.tar.gz")
		if os.path.exists("data.tar.gz"):
			os.unlink("data.tar.gz")

		f = open("control", "w")
		for k in self.control.keys():
			f.write("{}: {}\n".format(k, self.control[k]))
		f.close()

		tar = tarfile.open("control.tar.gz", "w:gz")
		tar.add("control")
		tar.close()

		tar = tarfile.open("data.tar.gz", "w:gz")
		if data_files:
			for df in data_files:
				tar.add(df)
		tar.close()


		if tar_not_ar:
			tar = tarfile.open(filename, "w:gz")
			tar.add("control.tar.gz")
			tar.add("data.tar.gz")
			tar.close()
		else:
			os.system("ar q {} control.tar.gz data.tar.gz \
					2>/dev/null".format(filename))

		os.unlink("control")
		os.unlink("control.tar.gz")
		os.unlink("data.tar.gz")

import hashlib
def md5sum_file(fname):
    f = open(fname, 'rb')
    md5 = hashlib.md5()
    chunk_size = 4096
    for chunk in iter(lambda: f.read(chunk_size), b''):
        md5.update(chunk)
    return md5.hexdigest()

class OpkGroup:
	def __init__(self):
		self.opk_list = []

	def add(self, **control):
		self.opk_list.append(Opk(**control))

	def addOpk(self, opk):
		self.opk_list.append(opk)

	def write_opk(self, tar_not_ar=False):
		for o in self.opk_list:
			o.write(tar_not_ar)

	def write_list(self, filename="Packages"):
		f = open(filename, "w")
		for opk in self.opk_list:
			for k in opk.control.keys():
				f.write("{}: {}\n".format(k, opk.control[k]))
			fpattern = "{Package}_{Version}_{Architecture}.opk"
			fname = fpattern.format(**opk.control)
			md5sum = md5sum_file(fname)
			f.write("Filename: {}\n".format(fname))
			f.write("MD5Sum: {}\n".format(md5sum))
			f.write("\n")
		f.close()

def fail(msg):
	print("%s: Test failed: %s" % (__appname, msg))
	exit(-1)

# Expected failure: For issues we know haven't been fixed yet. As these aren't
# regressions don't return an error condition.
def xfail(msg):
	print("%s: Expected failure: %s" % (__appname, msg))
	exit(0)

def regress_init():
	"""
	Initialisation and sanity checking.
	"""

	if not os.access(cfg.opkgcl, os.X_OK):
		fail("Cannot exec {}".format(cfg.opkgcl))

	try:
		os.makedirs(cfg.opkdir)
	except OSError as exception:
		if exception.errno != errno.EEXIST:
			raise

	os.chdir(cfg.opkdir)

	os.system("rm -fr {}".format(cfg.offline_root))

	os.makedirs("{}/etc/opkg".format(cfg.offline_root))
	f = open("{}/etc/opkg/opkg.conf".format(cfg.offline_root), "w")
	f.write("arch all 1\n")
	f.write("src test file:{}\n".format(cfg.opkdir))
	f.close()
