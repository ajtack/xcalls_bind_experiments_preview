#
# Parses the mutex trace data from BIND and produces from it the
# N worst mutexes in the program. Badness is defined by a high
# ratio of block time to hold-time. The trace data should be given
# by standard input; just run BIND with -D.
#
NumberResultsReturned = ARGV[0].to_i

if NumberResultsReturned.nil?
	raise ArgumentError, "User should provide a number of results" +
		" to return as the only parameter."
end

# Skip everything until "Mutex stats"
while (aLine = $stdin.gets) 
	if aLine.include?("Mutex stats")
		break
	end
end

# Each line is split into its five fields.
class MutexStatLine
	attr_reader :fileName, :lineNumber, :hits, :timeHeld, :timeBlocked
	include Comparable
	
	def initialize(aLine)
		piece = aLine.split(nil)
		@fileName = piece[0]
		@lineNumber = piece[1].to_i
		@hits = piece[2].to_i
		@timeHeld = piece[3].to_f
		@timeBlocked = piece[4].to_f
	end
	
	def blockRatio
		if (timeHeld > 0)
			timeBlocked / timeHeld
		else
			0
		end
	end
	
	def <=>(anotherMsl)
		self.blockRatio <=> anotherMsl.blockRatio
	end
	
	def to_s
		"#{fileName}##{lineNumber}\t\t #{hits}\t#{timeHeld}\t#{timeBlocked}:\t#{blockRatio}"
	end
end

# Gather individual lines from the file.
line = Array.new
while (aLine = $stdin.gets)
	line.push(MutexStatLine.new(aLine))
end

def maximum (a, b)
	if a > b then a else b end
end
# Print the N worst.
line.delete_if do |theLine|
	maximum(theLine.timeHeld, theLine.timeBlocked) < 0.001
end

line.sort!
def minimum (a, b)
	if a < b then a else b end
end

minimum(NumberResultsReturned, line.length).times do |time|
	puts line.pop
end

