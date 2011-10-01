#!/usr/bin/python
# designed for Python 2.6, may work with other versions
# (C)2010 Kenneth Boyd, license: MIT.txt

from sys import argv,exit;

#   This is inspired by Perl's Plain Old Documentation format, but 
# works generically with languages that use shell one-line comments
# (e.g.: Python, Perl, PHP, Ruby, C/C++
#   Idea is that lines #/* cut-cut_id */ act like =cut in Perl.
#   C,C++ are fine because the C-comment becomes whitespace, leaving behind 
# a null preprocessing directive which is removed by the preprocessor.
#   Languages with native shell comments just treat these lines as comments

# flip-side: suppress_cut.py, which just suppresses these comments.

def SelectiveCut(cut_id,srcfile,destfile):
	test_lines = []
	old_line_ok = 1
	line_ok = 1
	TargetFile = open(destfile,'w')	# *.sh
	with open(srcfile,'r') as f:
		for line in f:
			line_proxy = line.strip()
			if line_proxy=='#/*cut-'+cut_id+'*/':
				line_ok = 1-line_ok
			if line_ok and old_line_ok:
				TargetFile.write(line)
			old_line_ok = line_ok
	TargetFile.close()


if __name__ == '__main__':
	# online help
	if 4!=len(argv):
		print "Usage: cut_id srcfile destfile"
		print "\texcises text lines contained between #/*cut-cut_id*/ comments"
		exit(0)

	SelectiveCut(argv[1],argv[2],argv[3])

