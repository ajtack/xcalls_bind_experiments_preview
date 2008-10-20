#
# Accepts a newline-separated list of domains from standard input, and spits out a
# parameterized number of queries using random selection from this set of domains.
#
numberToGenerate = 0
if ARGV.length > 0 and not ARGV[0].nil?
	numberToGenerate = ARGV[0].to_i
else
	raise ArgumentError, "User must provide a number of queries to generate as the only argument to this script."
end

Domains = $stdin.readlines

# Verify input
Domains.each do |domain|
	if not domain =~ /([-\w\.]+)/
		raise ArgumentError, "'#{domain}' from standard input is not a valid domain!"
	else
		domain.strip!
	end
end

numberToGenerate.times do
	index = Kernel.rand(Domains.length)
	domain = Domains[index]
	
	if domain =~ /\d+\.\d+\.\d+\.\d+?/
		puts "#{domain}.in-addr.arpa PTR"
	else
		puts "#{Domains[index]} A"
	end
end
