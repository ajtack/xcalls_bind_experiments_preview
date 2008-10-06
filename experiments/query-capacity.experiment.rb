#
# Runs queryperf against a running instance of the BIND daemon,
# aggregating the average number of queries per second from
# each run.
#
# Arguments:
BindRoot        =	if ARGV[0][-1..-1] == '/'
                  then ARGV[0][0..-2]
                  else ARGV[0] end
ExperimentsRoot = if ARGV[1][-1..-1] == '/'
                  then ARGV[1][0..-2]
                  else ARGV[1] end
Port            = ARGV[2].to_i
Configuration   = ARGV[3]
Repetitions     = ARGV[4].to_i

childPid = Kernel.fork
if childPid.nil?
	# We are the child.
	# Start BIND.
	exec("#{BindRoot}/bin/named/named -p #{Port} -c #{Configuration} -f")
else
	# We are the parent
	# Run experiment
	totalQps = 0
	Repetitions.times do |repNumber|
		IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
		         "-p #{Port} -d #{ExperimentsRoot}/queries.dat")                do |result|
			totalQps += result.readlines[-2].split[3].to_i
			# puts result.readlines[-2].split[3].to_i
		end
	end

	# Kill BIND
	Process.kill(3, childPid)
	puts totalQps / Repetitions
end

