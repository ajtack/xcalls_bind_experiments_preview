# Accepts a newline-separated list of domains from standard input and generates
# from it an autoritative zone file for experiments.

require 'socket'

Domains = $stdin.readlines

# Verify input
Domains.each do |domain|
	if not domain =~ /([-\w\.]+)/
		raise ArgumentError, "'#{domain}' from standard input is not a valid domain!"
	else
		domain.strip!
	end
end

puts "$TTL 3h"
puts <<SOA
. IN SOA #{Socket.gethostname}. tack.cs.wisc.edu. (
	                          1        ; Serial
	                          3h       ; Refresh after 3 hours
	                          1h       ; Retry after 1 hour
	                          1w       ; Expire after 1 week
	                          1h )     ; Negative caching TTL of 1 hour)
SOA
puts ".\tIN NS\t#{Socket.gethostname}"

Domains.each do |domain|
	if domain =~ /\d+\.\d+\.\d+\.\d+?/
		puts "#{domain}.in-addr.arpa.\tIN PTR\tsomething.com."
	else
		octets = [Kernel.rand(256), Kernel.rand(256), Kernel.rand(256), Kernel.rand(256)]
		ipAddress = octets.join('.')
		puts "#{domain}.\tIN A\t#{ipAddress}"
	end
end
