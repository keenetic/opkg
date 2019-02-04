import tarfile, os, sys, stat
import cfg
import errno

__appname = sys.argv[0]

class Opk:
	valid_control_fields = ["Package", "Version", "Depends", "Provides",\
			"Replaces", "Conflicts", "Suggests", "Recommends",\
			"Section", "Architecture", "Maintainer", "MD5Sum",\
			"Size", "InstalledSize", "Filename", "Source",\
			"Description", "OE", "Homepage", "Priority",\
			"Conffiles", "Essential"]

	control = None
	preinst = None
	postinst = None
	prerm = None
	postrm = None

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

	def write(self, tar_not_ar=False, data_files=None, compression='gz'):
		COMPRESSORS = ['gz', 'bz2', 'xz']
		if compression not in COMPRESSORS:
			raise Exception("Invalid compression type: "
				"{}. Supported compressors: {}"\
				.format(compression, ", ".join(COMPRESSORS)))

		control_file = 'control.tar.' + compression
		data_file = 'data.tar.' + compression
		tar_mode = "w:" + compression

		TEMP_FILES = ['control', control_file, data_file,
			'preinst', 'postinst', 'prerm', 'postrm',
			'debian-binary']

		filename = "{Package}_{Version}_{Architecture}.opk"\
						.format(**self.control)

		for f in TEMP_FILES + [filename]:
			if os.path.exists(f):
				os.unlink(f)

		with open("debian-binary", "w") as f:
			f.write("2.0\n")

		with open("control", "w") as f:
			for k in self.control.keys():
				f.write("{}: {}\n".format(k, self.control[k]))

		if self.preinst:
			with open("preinst", "w") as f:
				os.fchmod(f.fileno(), 0o755)
				f.write(self.preinst)

		if self.postinst:
			with open("postinst", "w") as f:
				os.fchmod(f.fileno(), 0o755)
				f.write(self.postinst)

		if self.prerm:
			with open("prerm", "w") as f:
				os.fchmod(f.fileno(), 0o755)
				f.write(self.prerm)

		if self.postrm:
			with open("postrm", "w") as f:
				os.fchmod(f.fileno(), 0o755)
				f.write(self.postrm)

		with tarfile.open(control_file, tar_mode) as tar:
			tar.add("control")
			if self.preinst: tar.add("preinst")
			if self.postinst: tar.add("postinst")
			if self.prerm: tar.add("prerm")
			if self.postrm: tar.add("postrm")

		with tarfile.open(data_file, tar_mode) as tar:
			if data_files:
				for df in data_files:
					tar.add(df)

		if tar_not_ar:
			with tarfile.open(filename, tar_mode) as tar:
				tar.add(control_file)
				tar.add(data_file)
		else:
			os.system("ar q {0} {1} {2} {3}\
					2>/dev/null".format(filename,
						'debian-binary',
						control_file, data_file))

		for f in TEMP_FILES:
			if os.path.exists(f):
				os.unlink(f)

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
		opk = Opk(**control)
		self.opk_list.append(opk)
		return opk

	def addOpk(self, opk):
		self.opk_list.append(opk)

	def removeOpk(self, opk):
		self.opk_list.remove(opk)

	def write_opk(self, tar_not_ar=False, compression='gz'):
		for o in self.opk_list:
			o.write(tar_not_ar=tar_not_ar, compression=compression)

	def write_list(self, filename="Packages"):
		f = open(filename, "w")
		for opk in self.opk_list:
			for k in opk.control.keys():
				f.write("{}: {}\n".format(k, opk.control[k]))
			fpattern = "{Package}_{Version}_{Architecture}.opk"
			fname = fpattern.format(**opk.control)
			fsize = os.stat(fname).st_size
			md5sum = md5sum_file(fname)
			f.write("Filename: {}\n".format(fname))
			f.write("Size: {}\n".format(fsize))
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

	confdir=os.environ['SYSCONFDIR']+"/opkg";
	os.makedirs(("{}"+confdir).format(cfg.offline_root))
	f = open(("{}"+confdir+"/opkg.conf").format(cfg.offline_root), "w")
	f.write("arch all 1\n")
	f.write("src test file:{}\n".format(cfg.opkdir))
	f.close()
