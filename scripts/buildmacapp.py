#!/usr/bin/env python

import os,dircache,shutil,errno,sys,re;
from plistlib import Plist
from distutils import dir_util, file_util
import xml.etree.ElementTree as ET
sys.path.insert(1, os.path.join(sys.path[0], '..'))
import version

dest = "./build"
app_name = "Horizon"
binaries = ["horizon-eda","horizon-imp","horizon-pool","horizon-prj"]
projectdir = "./bundle"

brewprefix = os.popen("brew --prefix").read().splitlines()[0]
print "Brew prefix %s" % ( brewprefix )

def recursive_rm(dirname):
    if not os.path.exists(dirname):
        return
    
    files = dircache.listdir(dirname)
    for file in files:
        path = os.path.join (dirname, file)
        if os.path.isdir(path):
            recursive_rm(path)
        else:
            retval = os.unlink(path)
    if (os.path.islink(dirname)):
        os.unlink(dirname)
    else:
        os.rmdir(dirname)

recursive_rm(os.path.join(dest, app_name + ".app"))


def get_project_path(*args):
	return os.path.join(projectdir, *args)

def get_brew_path(*args):
	return str(os.path.join(brewprefix, *args))

plist = Plist.fromFile(get_project_path("Info.plist"))

def get_bundle_path(*args):
	bundle_path = os.path.join(dest, app_name + ".app")
	return os.path.join(bundle_path, *args)

def makedirs(path):
	if not os.path.exists(path):
		os.makedirs(path)

def create_skeleton():
    makedirs(get_bundle_path("Contents/Resources"))
    makedirs(get_bundle_path("Contents/MacOS"))
    makedirs(get_bundle_path("Contents/Resources/bin"))
    makedirs(get_bundle_path("Contents/Resources/share"))
    makedirs(get_bundle_path("Contents/Resources/lib"))

def create_pkglist():
	path = get_bundle_path("Contents", "PkgInfo")
	f = open (path, "w")
	f.write(plist.CFBundlePackageType)
	f.write(plist.CFBundleSignature)
	f.close()

def create_info_plist():
    path = get_bundle_path("Contents", "Info.plist")
    plist.CFBundleVersion = version.string
    plist.CFBundleShortVersionString = version.string
    plist.write(path)

alllibs = set()
def add_lib(libfile):
	if not libfile in alllibs:
		alllibs.add(libfile)
		print "Add lib %s" % (libfile)
		filename = os.path.basename(libfile)
		dest = get_bundle_path("Contents/Resources/lib",filename)
		file_util.copy_file (str(libfile), str(dest),
                                            preserve_mode=1,
                                            preserve_times=1,
                                            update=1,
                                            link=None,
                                            verbose=1,
                                            dry_run=0)
		rebase_lib(str(dest),str(libfile))
		return str(filename)

def change_rpath(libfile,srcfile):
	dst = os.path.basename(libfile)
	os.chmod(libfile, 0644)
	cmd = "install_name_tool -change " + str(srcfile) + " " + str(os.path.join("@executable_path/../lib",dst)) + " " + libfile
	print("Exec %s" % (cmd))
	f = os.popen(cmd)
	for line in f:
		print(f)
	f.close()

def change_libpath(binfile,libfile):
	dst = os.path.basename(libfile)
	if binfile.endswith(".dylib") or binfile.endswith(".so"):
		os.chmod(binfile, 0644)
	cmd = "install_name_tool -change " + str(libfile) + " " + str(os.path.join("@executable_path/../lib",dst)) + " " + binfile
	print("Exec %s" % (cmd))
	f = os.popen(cmd)
	for line in f:
		print(f)
	f.close()

def rebase_lib(destfile,srcfile):
	print "Rebase lib %s" % (destfile)
	f = os.popen("otool -L " + destfile)
	lines = [line.strip() for line in f]
	p = re.compile("(.*\.dylib\.?.*)\s\(compatibility.*$")
	rebase = list()
	for line in lines:
		match = p.match(line)
		if match:
			libfile = match.group(1)
			if libfile.startswith(srcfile): 
				None
			elif libfile.startswith("/usr/lib/"):
				None
			elif libfile.startswith("/System/Library/Frameworks/"):
				None
			elif libfile.startswith("@executable_path/"):
				None
			elif libfile.startswith("@loader_path/"):
				basedir = os.path.dirname(srcfile)
				add_lib(libfile.replace("@loader_path",basedir))
			else:
				add_lib(libfile)
				rebase.append(libfile)
				
	f.close()
	print "End rebase lib %s" % (destfile)
	for libfile in rebase:
		change_libpath(destfile,libfile)
	#change_rpath(destfile,srcfile)





def rebase_bin(destfile):
	print "Rebase bin %s" % (destfile)
	f = os.popen("otool -L " + destfile)
	lines = [line.strip() for line in f]
	p = re.compile("(.*\.dylib\.?.*)\s\(compatibility.*$")
	for line in lines:
		match = p.match(line)
		if match:
			libfile = match.group(1)
			if libfile.startswith(destfile): 
				None
			elif libfile.startswith("/usr/lib/"):
				None
			elif libfile.startswith("/System/Library/Frameworks/"):
				None
			else:
				add_lib(libfile)
				change_libpath(destfile,libfile)

copied_binaries = list()
def copy_bin(src_dir,file,dst_dir):
	source = os.path.join(src_dir,file)
	dest = get_bundle_path(dst_dir,file)
	file_util.copy_file (str(source), str(dest),
                                            preserve_mode=1,
                                            preserve_times=1,
                                            update=1,
                                            link=None,
                                            verbose=1,
                                            dry_run=0)
	copied_binaries.append(str(dest))
	rebase_bin(str(dest))

def copytree(src, dst, symlinks=False, ignore=None):
	makedirs(dst)
	for item in os.listdir(src):
	    s = os.path.join(src, item)
	    d = os.path.join(dst, item)
	    if os.path.isdir(s):
	        shutil.copytree(s, d, symlinks, ignore)
	    else:
	        shutil.copy2(s, d)

def copy_data(src_dir,dst_dir):
	try:
		print "Copying %s to %s" % (src_dir, dst_dir)
		copytree(str(src_dir), str(dst_dir))
	except EnvironmentError, e:
		if e.errno == errno.ENOENT:
			print "Warning, source file missing: " + src_dir
		elif e.errno == errno.EEXIST:
			print "Warning, path already exits: " + dest
		else:
			print "Error %s when copying file: %s" % ( str(e), src_dir )
		sys.exit(1)

def parse_ui(ui_filepath,iconsset):
    print "Parse UI %s" % (ui_filepath)
    root = ET.parse(ui_filepath).getroot()
    for prop in ['primary_icon_name','icon_name']:
        for type_tag in root.findall(".//property[@name='"+prop+"']"):
            value = type_tag.text
            print "Found icon usage %s" % (value)
            iconsset.add(value)

def copy_icon_themes(themes):
    all_icons = set()

    
    for theme in themes:
    	name = os.path.basename(theme)
    	makedirs(get_bundle_path('Contents/Resources/share/icons',name))
    	file_util.copy_file (str(os.path.join(theme, "index.theme")), str(get_bundle_path('Contents/Resources/share/icons',name,'index.theme')),
                                            preserve_mode=1,
                                            preserve_times=1,
                                            update=1,
                                            link=None,
                                            verbose=1,
                                            dry_run=0)
   
    strings = set()
    for theme in themes:
        for root, dirs, files in os.walk(theme):
            for f in files:
                (head, tail) = os.path.splitext(f)
                if tail in [".png", ".svg"]:
                    all_icons.add(head)
                    category = os.path.basename(root)
                    if category in ["ui"]: # add all ui
                    	strings.add(head)

    

    # Get strings from binaries.
    for f in copied_binaries:
        p = os.popen("strings " + f)
        for string in p:
            string = string.strip()
            strings.add(string)

    # Also get strings from glade files.

    for root, dirs, files in os.walk("./src"):
    	for f in files:
    		(head,tail) = os.path.splitext(f)
    		if tail in [".ui"]:
    			parse_ui(os.path.join(root,f),strings)


    used_icons = all_icons.intersection(strings)
    
    for theme in themes:
        prefix = os.path.dirname(theme)
        name = os.path.basename(theme)
        for root, dirs, files in os.walk(theme):
            for f in files:
                # Go through every file, if it matches the icon
                # set, copy it.
                (head, tail) = os.path.splitext(f)

                if head.endswith('.symbolic'):
                    (head, tail) = os.path.splitext(head)

                if head in used_icons:
                    path = os.path.join(root, f)

                    # Note: Skipping svgs for now, they are really
                    # big and not really used.
                    #if not path.endswith(".svg"):
                    #   continue

                    # Replace the real paths with the prefix macro
                    # so we can use copy_path.
                    dst = path[len(theme)+1:]
                    #print(name,dst)
                    icondst = get_bundle_path('Contents/Resources/share/icons',name,dst)
                    makedirs(os.path.dirname(icondst))
                    file_util.copy_file (str(path), str(icondst),
                                            preserve_mode=1,
                                            preserve_times=1,
                                            update=1,
                                            link=None,
                                            verbose=1,
                                            dry_run=0)
                    
                    
    # Generate icon caches.
    for theme in themes:
    	name = os.path.basename(theme)
        path = get_bundle_path("Contents/Resources/share/icons", name)
        cmd = "gtk3-update-icon-cache -f " + path 
        f = os.popen(cmd)
        for l in f: 
        	print(l)

def create_gdk_pixbuf_loaders_setup():
        modulespath = ""
        cachepath = ""
        gdk_pixbuf_binary_version = "2.10.0"
        sourcepath = get_brew_path("lib","gdk-pixbuf-2.0",gdk_pixbuf_binary_version,"loaders")
       
       	modulesbundle = os.path.join("Contents/Resources/lib/",
                                                 "gdk-pixbuf-2.0", 
                                                 gdk_pixbuf_binary_version,
                                                 "loaders")
       	modulespath = get_bundle_path(modulesbundle)
       	makedirs(modulespath)
       	loaders = list()

        for root, dirs, files in os.walk(sourcepath):
            for f in files:
            	print "Found pixbuf module %s" % (f)
                (head, tail) = os.path.splitext(f)
                if tail in [".so"]:
                	copy_bin(sourcepath,f,modulesbundle)
                	loaders.append(" " + os.path.join(sourcepath,f))

        
        
        cachepath = get_bundle_path("Contents/Resources/lib/",
                                                 "gdk-pixbuf-2.0",
                                                 gdk_pixbuf_binary_version,
                                                 "loaders.cache")
          
        
        cmd = "gdk-pixbuf-query-loaders" + ''.join(loaders)
        print(cmd)
        f = os.popen(cmd)

        makedirs(os.path.dirname(cachepath))
        fout = open(cachepath, "w")

        prefix = "\"" + get_brew_path("")
        for line in f:
            print(line)
            line = line.strip()
            if line.startswith("#"):
                continue

            # Replace the hardcoded bundle path with @executable_path...
            if line.startswith(prefix):
            	line = line[len(prefix):]
                filename = os.path.basename(line)
                #line="\"" + filename
                line = "\"@executable_path/../lib/gdk-pixbuf-2.0/" + gdk_pixbuf_binary_version + "/loaders/" + filename
            fout.write(line)
            fout.write("\n")
        fout.close()

create_skeleton()
create_pkglist()
create_info_plist()

file_util.copy_file (str(get_project_path("launcher.sh")), str(get_bundle_path("Contents/MacOS","launcher.sh")),
                                            preserve_mode=1,
                                            preserve_times=1,
                                            update=1,
                                            link=None,
                                            verbose=1,
                                            dry_run=0)

for file in binaries:
	copy_bin("./build",file,"Contents/Resources/bin")

create_gdk_pixbuf_loaders_setup()

copy_data(get_brew_path("share/glib-2.0/schemas"), get_bundle_path("Contents/Resources/share/glib-2.0/schemas"))
copy_data(get_brew_path("share/themes/Mac"), get_bundle_path("Contents/Resources/share/themes/Mac"))

copy_data("./src/icons/scalable/apps",get_bundle_path("Contents/Resources/share/icons/hicolor/scalable/apps"))
copy_icon_themes([get_brew_path('share/icons/Adwaita'),get_brew_path('share/icons/hicolor')])
