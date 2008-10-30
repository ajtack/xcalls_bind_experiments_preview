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
Logged          = (ARGV[5] == '--logged')


IO.popen('-') do |pipe|
	if pipe.nil?
		startBind(Logged) do |bindIn, bindOut, bindErr|
			puts "Started BIND!"
			
			trap('INT') {}
			puts "BIND has been killed."
		end
	else
		#Wait for BIND to start
		$stderr.puts pipe.gets
		
		# Run experiment
		totalQps = 0
		Repetitions.times do |repNumber|
			IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
			         "-p #{Port} -d #{ExperimentsRoot}/queries.dat -q 400 -l 20")                do |result|
				$stderr.puts "Running Queryperf excercise #{repNumber} ..."
				qps = result.readlines[-2].split[3].to_i
				totalQps += qps
			end
		end
		
		killBind
		$stderr.puts pipe.gets
		$stderr.puts "#{totalQps / Repetitions} queries per second"
		puts "#{totalQps / Repetitions} queries per second"
	end
end
