#!/usr/bin/python2.4

import os, sys, getopt, subprocess, string
import datetime, time
import signal

stdoutFile = ""
stderrFile = ""
logFile = ""
timeout = 60   # seconds

def alarmHandler(signum, frame):
  pass

def runProg(args):
	argstr = ""
	for arg in args:
		argstr = argstr + arg + " "
	signal.alarm(timeout)
	p = subprocess.Popen(argstr.split(" "), shell=False,
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
	try:
		ret = os.waitpid(p.pid, 0)
		signal.alarm(0)
	except OSError:
		os.system("kill -9 %d" % p.pid)
		ret = [0, 9]
	t = datetime.datetime.now()
	now = datetime.datetime.fromtimestamp(time.mktime(t.timetuple()))
	if logFile != "":
		flog = open(logFile, 'a')
	if ret[1] != 9:
		if stdoutFile != "":
			fout = open(stdoutFile, 'a')
			fout.writelines(p.stdout.readlines())
		else:
			for l in p.stdout.readlines():
				sys.stdout.write(p.stdout.readlines())
		if stderrFile != "":
			ferr = open(stderrFile, 'a')
			ferr.writelines(argstr + '\n')
			ferr.writelines(p.stderr.readlines())
		else:
			for l in p.stderr.readlines():
				sys.stderr.write(l)
		if (ret[1] == 0):
			logmsg = now.ctime() + " # " + argstr + " # SUCCESS\n"
		else:
			logmsg = now.ctime() + " # " + argstr + " # FAILURE\n"
		if logFile != "":
			flog.writelines(logmsg)
		else:
			sys.stderr.write(logmsg)
		if stdoutFile != "":
			fout.close()
		if stderrFile != "":
			ferr.close()
	else:
		if logFile != "":
			flog.writelines(now.ctime() + " # " + argstr + " # TIMEOUT\n")
	if logFile != "":
		flog.close()
	return ret[1] 
            

def usage():
	print "Usage: run.py [options] <program_name> <parameters_list>" 
	print "Options:"
	print "\t-o, --stdout\t\tRedirect standard out to given file"    
	print "\t-r, --stderr\t\tRedirect standard error to given file"    
	print "\t-g, --log\t\tError logging file"    
	print "\t-t, --timeout\t\tTimeout for killing a spawned run"    
	print "\t-h, --help\t\tPrints this help message"    

def main(argv):                         
	global timeout, stdoutFile, stderrFile, logFile
	global fout, flog
	try:                                
		opts, args = getopt.getopt(argv, "o:r:g:t:h", ["stdout=", "stderr=", "log=", "timeout=", "help"])
	except getopt.GetoptError:
		usage()
		sys.exit(2)                     
	for opt, arg in opts:
		if opt in ("-o", "--stdout"):
			stdoutFile = arg
		elif opt in ("-r", "--stderr"):
			stderrFile = arg
		elif opt in ("-g", "--log"):
			logFile = arg
		elif opt in ("-t", "--timeout"):
			timeout = string.atoi(arg)
		elif opt in ("-h", "--help"):
			usage()
			sys.exit(2)
	if len(args)==0:
		usage()
		sys.exit(2)
	return runProg(args)

if __name__ == "__main__":
	signal.signal(signal.SIGALRM, alarmHandler)
	ret = main(sys.argv[1:])
	if ret != 0:
		sys.exit(1)
