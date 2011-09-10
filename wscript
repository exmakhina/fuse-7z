#!/usr/bin/env python

program="fuse-7z"
version="0.1"

out = "build"

def options(opt):
	opt.load("compiler_cxx compiler_c")

def configure(conf):
	conf.load("compiler_cxx compiler_c")
	conf.check_cfg(
	 package="fuse",
	 args=['--atleast-version=2.8'],
	 msg="Checking for fuse >=2.8",
	)
	conf.check_cfg(
	 package="fuse",
	 args=['--cflags', '--libs'],
	 msg="Checking for fuse flags",
	 uselib_store="FUSE",
	)
	conf.env.CFLAGS += ["-std=c99"]
	if 0:
		conf.env.LIBPATH_P7ZIP = ["/usr/lib64/p7zip"]
		conf.env.LIB_P7ZIP = ["7z"]
	if 0:
		conf.env.INCLUDES_LIB7ZIP = "lib7zip/Lib7Zip"
		conf.env.LINKFLAGS_LIB7ZIP = [
		 "-L", conf.path.find_or_declare("lib7zip/Lib7Zip").bldpath(),
		 "-Wl,-Bstatic", "-l","7zip", "-Wl,-Bdynamic",
		]

def build(bld):
	p7z_root = bld.path.find_dir("p7zip_9.20.1")

	if 0: # when I'll have the courage to use p7zip directly
		src = p7z_root.ant_glob("**/**.c")
		src += p7z_root.ant_glob("CPP/7zip/Archives/**.cpp")
		src += p7z_root.ant_glob("CPP/7zip/Common/**.cpp")
		src += p7z_root.ant_glob("CPP/7zip/Compress/**.cpp")
		src += p7z_root.ant_glob("CPP/7zip/Crypto/**.cpp")
		src += p7z_root.ant_glob("CPP/Common/**.cpp")
		src += p7z_root.ant_glob("CPP/myWindows/**.cpp")
		src += p7z_root.ant_glob("CPP/Windows/PropVariant.cpp")
		src += p7z_root.ant_glob("CPP/Windows/Synchronization.cpp")
		#src += p7z_root.ant_glob("CPP/Windows/FileDir.cpp")
		src += p7z_root.ant_glob("CPP/Windows/FileFind.cpp")
		src += p7z_root.ant_glob("CPP/Windows/FileIO.cpp")
		forb = ["test", "7zCrcT8", "Lang", "ListFileUtils", "DllExports.cpp"]
		for forbidden in forb:
			src = [ x for x in src if forbidden not in x.abspath() ]
	src = p7z_root.ant_glob("CPP/Windows/PropVariant.cpp")
	src += p7z_root.ant_glob("CPP/Common/MyWindows.cpp")
	inc = [ p7z_root.find_dir(x) for x in [".", "CPP", "CPP/include_windows", "CPP/myWindows", "C"] ]
	bld(
	 target="p7zip",
	 features="cxx",
	 source=src,
	 includes = inc,
	 export_includes = inc,
	 use = "P7ZIP",
	 defines=["NEED_NAME_WINDOWS_TO_UNIX=1"],
	)

	bld(
	 target="lib7zip",
	 features="cxx",
	 source=[ bld.path.find_or_declare("lib7zip/Lib7Zip/%s" %x ) for x in ["7ZipArchive.cpp", "7ZipArchiveItem.cpp", "7zipLibrary.cpp", "HelperFuncs.cpp"]],
	 includes=inc,
	 export_includes="lib7zip/Lib7Zip",
	 use=[
	  "p7zip",
	  "P7ZIP",
	 ],
	)
	
	bld(
	 target="fuse-7z-lib",
	 features="cxx",
	 source=[
	  "fuse-7z.cpp",
	 ],
	 use=[
	  "FUSE",
	  #'LIB7ZIP',
	  #"lib7zip",
	  #"p7zip",
	 ],
	 defines=["PROGRAM=\"%s\"" % program, "VERSION=\"%s\"" % version],
	)

	bld(
	 target="fuse-7z-node",
	 features="cxx",
	 source=[
	  "fuse-7z-node.cpp",
	 ],
	 use=[
	 ],
	)

	bld(
	 target="fuse-7z-7zip",
	 features="cxx",
	 source=[
	  "fuse-7z-7zip.cpp",
	 ],
	 use=[
	  "lib7zip",
	 ],
	 defines=["PROGRAM=\"%s\"" % program, "VERSION=\"%s\"" % version],
	)

	bld(
	 target="main",
	 features="c",
	 source=[
	  "main.c",
	 ],
	 use=[
	  "FUSE",
	 ],
	 defines=["PROGRAM=\"%s\"" % program, "VERSION=\"%s\"" % version],
	)

	bld(
	 target="fuse-7z",
	 features="c cxxprogram",
	 source=[
	 ],
	 use=[
	  "fuse-7z-7zip",
	  "fuse-7z-lib",
	  "fuse-7z-node",
	  "main",
	 ],
	)

