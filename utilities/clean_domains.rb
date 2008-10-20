#
# Runs queryperf against a running instance of the BIND daemon,
# aggregating the average number of queries per second from
# each run.
#
require 'open3'
require 'set'
require File.dirname(__FILE__) + '/bind'

BindRoot        =	if ARGV[0][-1..-1] == '/'
                  then ARGV[0][0..-2]
                  else ARGV[0] end
ExperimentsRoot = if ARGV[1][-1..-1] == '/'
                  then ARGV[1][0..-2]
                  else ARGV[1] end
Port            = ARGV[2].to_i
Configuration   = ARGV[3]


Domains = $stdin.readlines.compact.uniq


IO.popen("-", "r+") do |pipe|
	if pipe.nil?
		startBind do |bindIn, bindOut, bindErr|
			$stdout.puts "Started BIND!"
			
			# BIND will be ended by an interrupt signal from the parent:
			# Note race condition here!
			trap('INT')	{}
			
			# Look for any domains or IP addresses in the BIND error output.
			DomainNameQuery = /'([\w+\.]+\w+[^\.])\/A\/IN'/
			IpAddressQuery = /'([\d+\.]{3}[\d^\.]+)\/IN'/
			badUrls = Array.new
			bindErr.readlines.each do |line|
				badUrls += line.scan(DomainNameQuery)
				badUrls += line.scan(IpAddressQuery)
			end
			
			# Give the filtered domains back to the parent
			puts badUrls.compact.uniq.sort.join('\n')
		end
	else
		# Convert the domains to queryperf queries
		queries = Domains.collect do |domain|
			domain.strip!
			if domain =~ /[\d+\.]{3}[\d^\.]+/
				"#{domain}.in-addr.arpa PTR\n"
			else
				"#{domain} A\n"
			end
		end
		
		# Wait for the other thread to get BIND ready
		$stderr.puts pipe.gets
		
		# Run queryperf against BIND!
		IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
		         "-p #{Port} -q 400", "r+") do |queryperfStdio|
			$stderr.puts "Started queryperf!"
			queryperfStdio.write(queries)
			queryperfStdio.close_write
			$stderr.puts "Waiting for queryperf to finish..."
		end
		
		killBind
		
		# Get the bad URLs from the BIND child
		BadDomains = Set.new(pipe.read.split('\n'))
		AllDomains = Set.new(Domains)
		GoodDomains = Set.new(AllDomains - BadDomains)
		GoodDomains.sort.each { |domain| puts domain }
		$stderr.puts "Removed #{AllDomains.size - GoodDomains.size}/#{AllDomains.size} (#{BadDomains.size}) domains for failing to respond without error."
	end
end
