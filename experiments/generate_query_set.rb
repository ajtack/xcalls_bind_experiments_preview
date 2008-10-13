#
# Collects a set of random queries as long as the parameter given, and spits the
# newline-separated result out to stdout.
#
NumberToGenerate = ARGV[0].to_i

def nonzero_random(param)
	result = 0
	while result == 0
		result = Kernel.rand(param)
	end
	
	result
end

def String.random_alphanumeric(size=16)
	string = String.new
	alphanumerics = [('0'..'9'),('A'..'Z'),('a'..'z')].map {|range| range.to_a}.flatten
	
	while string.empty?
		string = (0...size).map { alphanumerics[Kernel.rand(alphanumerics.size)] }.join
	end
	
	string
end

NumberToGenerate.times do
	randomDomain1 = String.random_alphanumeric(nonzero_random(3 + 1))

	# 255 + 1 (rand gives less than the number given,
	# Subtract 1 for the '.', 1 for the terminator, and
	# 1 for some other character in the packet.
	randomDomain2 = String.random_alphanumeric(nonzero_random(256 - randomDomain1.length - 2))
	puts "#{randomDomain2}.#{randomDomain1} A"
end
