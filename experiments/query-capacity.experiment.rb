#
# Runs queryperf against a running instance of the BIND daemon,
# aggregating the average number of queries per second from
# each run.
#
# Arguments:
require File.dirname(__FILE__) + '/../utilities/bind'

BindRoot        =	if ARGV[0][-1..-1] == '/'
                  then ARGV[0][0..-2]
                  else ARGV[0] end
ExperimentsRoot = if ARGV[1][-1..-1] == '/'
                  then ARGV[1][0..-2]
                  else ARGV[1] end
Port            = ARGV[2].to_i
Configuration   = ARGV[3]
Repetitions     = ARGV[4].to_i


# Start BIND.
process = IO.popen("#{BindRoot}/bin/named/named -f -g -p #{Port} -c #{Configuration}")

# Run experiment
totalQps = 0
Repetitions.times do |repNumber|
	IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
	         "-p #{Port} -d #{ExperimentsRoot}/queries.dat -q 400")                do |result|
		totalQps += result.readlines[-2].split[3].to_i
	end
end

# Kill BIND
killBind
puts totalQps / Repetitions
