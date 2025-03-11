#!/usr/bin/python -B
#
# Copyright (C) 2017 and later: Unicode, Inc. and others.
# License & terms of use: http://www.unicode.org/copyright.html
#
# Copyright (C) 2013-2014 IBM Corporation and Others. All Rights Reserved.
#
print("NOTE: this tool is a TECHNOLOGY PREVIEW and not a supported ICU tool.")
#
# @author Steven R. Loomis <srl@icu-project.org>
#
# Yet Another Resource Builder
#
# Usage:
#
##$ mkdir loc
##$ echo 'root { hello { "Hello" } }' > loc/root.txt
##$ echo 'es { hello { "Hola" } }' > loc/es.txt
##$ rm -rf ./out
##$ bldicures.py --name myapp --from ./loc -d ./out
##$ ls out/myapp.dat
#
# use 'bldicures.py --help' for help.
#
# BUGS/TODO
# * dependency calculation
# * pathnames/PATH for genrb/pkgdata
# * cleanup of files, ./out, etc
# * document res_index
# * probably pathname munging
# * deuglify python


import sys

# for utf-8
#reload(sys)
#sys.setdefaultencoding("utf-8")

import argparse
import os

endian=sys.byteorder

parser = argparse.ArgumentParser(description='Yet Another ICU Resource Builder', epilog='ICU tool, http://icu-project.org - master copy at http://source.icu-project.org/repos/icu/tools/trunk/scripts/bldicures.py')
parser.add_argument('-f', '--from', action='append', dest='fromdirs', help='read .txt files from this dir', metavar='fromdir', required=True)
parser.add_argument('-n', '--name', action='store', help='set the bundle name, such as "myapp"', metavar='bundname', required=True)
parser.add_argument('-m', '--mode', action='store', help='pkgdata mode', metavar='mode', default="archive")
parser.add_argument('-d', '--dest', action='store', dest='destdir', help='dest dir, default is ".".', default=".", metavar='destdir')
parser.add_argument('-e', '--endian', action='store', dest='endian', help='endian, big, little or host, your default is "%s".' % endian, default=endian, metavar='endianness')
parser.add_argument('--verbose', '-v', action='count',default=0)

args = parser.parse_args()
if args.verbose>0:
    print("Options: "+str(args))

if args.verbose > 0:
    print("mkdir " + args.destdir)
os.makedirs(args.destdir)
tmpdir = 'tmp'
os.makedirs('%s/%s/' % (args.destdir, tmpdir))

listname = '%s/%s/icufiles.lst' % (args.destdir, tmpdir)



if args.endian not in ("big","little","host"):
    print("Unknown endianness: %s" % args.endian)
    sys.exit(1)

if args.endian == "host":
    args.endian = endian

needswap = args.endian is not endian

if needswap and args.mode not in ("archive", "files"):
    print("Don't know how to do swapping for mode=%s" % args.mode)
    sys.exit(1)

pkgmode = args.mode

if needswap and args.mode in ("files"):
    pkgmode = "archive"

if args.verbose > 0:
    print(">%s" % (listname))
listfn = open(listname, 'w')

print('# list for "%s" generated by %s on %s' % (args.name,parser.prog, '(now)'), file = listfn)
print('# args: ' + str(args), file = listfn)
print('#', file = listfn)

idxname = '%s/%s/res_index.txt' % (args.destdir, tmpdir)
idxfn = open(idxname, 'w')
print("""// Warning, this file is autogenerated by %s
res_index:table(nofallback) {
 InstalledLocales:table {""" % (parser.prog), file = idxfn)

gens = {}

def add_res(resname,txtname, loc):
    if args.verbose>0:
        print("+ %s (from %s)" % (loc, txtname))
    print("# %s" % (txtname), file = listfn)
    print("%s.res" % (loc), file = listfn)
    gens[loc] = { "loc": loc, "res": resname, "txt": txtname }

add_res('%s/%s/res_index.res' % (args.destdir,tmpdir), idxname, 'res_index')

def add_loc(path, loc):
    resf = '%s/%s/%s.res' % (args.destdir,tmpdir,loc)
    txtf = '%s/%s.txt' % (path, loc)
    print("  %s {\"\"}" % loc, file = idxfn)
    add_res(resf, txtf, loc)

for dir in args.fromdirs:
    if args.verbose>0:
        print("Collecting .txt files in %s" % (dir))

    walks = os.walk(dir)
    for ent in walks:
            junk = (path,dirs,files) = ent
            if args.verbose>3:
                print(junk)
            if (path.find("/.svn") != -1):
                continue
            for file in files:
                if (file[-4:] != ".txt"):
                    if args.verbose>1:
                        print("Ignoring %s/%s with suffix %s" % (path,file, file[-4:]))
                    continue
                loc = file[:-4]
                add_loc(path, loc)

print(" }", file = idxfn)
print("}", file = idxfn)

idxfn.close()
listfn.close()

if (args.verbose>2):
    print(gens)

if (args.verbose>3):
    print("TODO: dependency tracking. For now, don't care")

for gen in gens:
    item = gens[gen]
    cmd = 'genrb -d "%s/%s" "%s"' % (args.destdir, tmpdir, item["txt"])
    if (args.verbose>1):
        print("# " + cmd)
    os.system(cmd)

cmd = 'pkgdata -m "%s" -T "%s/%s" -p "%s" -s "%s/%s" -d "%s" "%s"' % (pkgmode,args.destdir,tmpdir,args.name,args.destdir,tmpdir,args.destdir,listname)
if (args.verbose>1):
    cmd = cmd + " -v"
    print("# " + cmd)
rc = os.system(cmd)
if rc != 0:
    print("# Command failed: " + cmd)
    sys.exit(rc)

if needswap:
    outfile = "%s/%s.dat" % (args.destdir, args.name)
    tmpfile =  "%s/%s/%s.dat" % (args.destdir, tmpdir, args.name)
    if args.mode in ("files","archive"):
        print("# %s -> %s" % (outfile, tmpfile))
        os.rename(outfile,tmpfile)
        # swap tmp back to out
        cmd = 'icupkg -w -tb "%s" "%s"' % (tmpfile, outfile)
        if (args.verbose>1):
            cmd = cmd + " -v"
            print("# " + cmd)
        rc = os.system(cmd)
        if rc != 0:
            print("# Swap command failed: " + cmd)
            sys.exit(rc)
    # fall through for files mode.
    if args.mode in ("files"):
        os.mkdir("%s/%s/" % (args.destdir, args.name))
        # unpack
        cmd = 'icupkg -tb -x "%s" -d "%s/%s/" "%s"' % (listname, args.destdir, args.name, outfile)
        if (args.verbose>1):
            cmd = cmd + " -v"
            print("# " + cmd)
        rc = os.system(cmd)
        if rc != 0:
            print("# Swap command failed: " + cmd)
            sys.exit(rc)
        # todo cleanup??
