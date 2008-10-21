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

class TransactionStatistics
 	attr_reader :transactions
	attr_reader :serialTransitions
	attr_reader :retries
	
	def initialize
		@transactions = Array.new
		@serialTransitions = Array.new
		@retries = Array.new
	end
	
	def record(results)
		if results.has_key? :transactions
			@transactions.push(results[:transactions])
			
			if results.has_key? :serialTransitions
				@serialTransitions.push(results[:serialTransitions])
			else
				@serialTransitions.push(0)
			end
			
			if results.has_key? :retries
				@retries.push(results[:retries])
			else
				@retries.push(0)
			end
		else
			raise "Tried to record nil results!"
		end
	end
	
	def average
		totalTransactions = 0
		@transactions.each do |number|
			totalTransactions += number
		end
		
		totalSerialTransitions = 0
		@serialTransitions.each do |number|
			totalSerialTransitions += number
		end
		
		totalRetries = 0
		@retries.each do |number|
			totalRetries += number
		end
		
		if @transactions.length == 0
			return {:transactions => 0, :serialTransitions => 0, :retries => 0}
		else
			return {
				:transactions => totalTransactions / @transactions.length,
				:serialTransitions => totalSerialTransitions / @serialTransitions.length,
				:retries => totalRetries / @retries.length
				}
		end
	end
end

# Takes the lines at the end of an ITM statistics report, converts the relevant figure
# to a floating point number, and returns a hash indicating the type of the result.
def pieceHash(pieceArray)
	case pieceArray[0]
	when 'Transactions':
		{ :transactions => pieceArray[2].to_f }
	when 'SerialTransitions'
		{ :serialTransitions => pieceArray[3].to_f }
	when 'Retries'
		{ :retries => pieceArray[3].to_f }
	else
		Hash.new
	end
end

# Run experiment
stats = TransactionStatistics.new
Repetitions.times do |repNumber|
	$stderr.puts "Running transaction-statistic measurement exercise #{repNumber} ..."
	IO.popen("-", "r+") do |pipe|
		if pipe.nil?
			startBind(Logged) do |bindIn, bindOut, bindErr|
				$stderr.puts "Started BIND!"

				# BIND will be ended by an interrupt signal from the parent:
				# Note race condition here!
				trap('INT')	{}
				puts "Bind Terminated!"
			end
		else
			# Wait for the other thread to get BIND ready
			$stderr.puts pipe.gets

			# Run queryperf against BIND!
			IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
			         "-p #{Port} -q 400 -d #{ExperimentsRoot}/queries.dat", "r") do |queryperfStdio|
				$stderr.puts "Started queryperf!"
				$stderr.puts "Waiting for queryperf to finish..."
			end

			# Kill BIND and synchronize such that itm.log has been created.
			killBind
			$stderr.puts pipe.gets

			# Read statistics
			until File.exist?("zones/itm.log")
				sleep(0.5)
			end
			logFile = File.open("zones/itm.log", "r")
			line = logFile.readlines
			File.delete("zones/itm.log")

			thisEntry = Hash.new
			line.count.downto 0 do |lineNumber|
				thisLine = line[lineNumber - 1]
				piece = thisLine.split

				if piece[0] == ':'  # line below "GRAND TOTAL ..."
					break
				else
					thisEntry.merge!(pieceHash(piece))
				end
			end

			stats.record(thisEntry)
		end
	end
end

puts "#{stats.average[:transactions]} transactions, " +
	"#{stats.average[:serialTransitions] * 100}% forced serial, " +
	"#{stats.average[:retries] * 100}% retried"
