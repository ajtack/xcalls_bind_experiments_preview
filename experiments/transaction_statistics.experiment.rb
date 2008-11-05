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
CpuCount        = ARGV[5].to_i
ReportFile      = ARGV[6]
Logged          = (ARGV[7] == '--logged')

require 'fileutils'

class TransactionStatistics
 	attr_reader :transactions
	attr_reader :serialTransitions
	attr_reader :retries
	
	def initialize
		@transactions = Array.new
		@serialTransitions = Array.new
		@retries = Array.new
		@userRetries = Array.new
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
			
			if results.has_key? :user_retries
				@userRetries.push(results[:user_retries])
			else
				@userRetries.push(0)
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
		
		totalUserRetries = 0
		@userRetries.each do |number|
			totalUserRetries += number
		end
		
		if @transactions.length == 0
			return {:transactions => 0, :serialTransitions => 0, :retries => 0, :userRetries => 0}
		else
			return {
				:transactions => totalTransactions / @transactions.length,
				:serialTransitions => totalSerialTransitions / @serialTransitions.length,
				:retries => totalRetries / @retries.length,
				:userRetries => totalUserRetries / @userRetries.length
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
		{ :retries => pieceArray[5].to_f }
	when 'UserRetries'
		{ :user_retries => pieceArray[5].to_f }
	else
		Hash.new
	end
end

if not File.exist?('zones/txc.ini')
	txcIniFile = File.new('zones/txc.ini', 'w')
	txcIniFile.puts 'statistics=enabled'
	txcIniFile.close
end

# Run experiment
stats = TransactionStatistics.new
Repetitions.times do |repNumber|
	$stderr.puts "Running transaction-statistic measurement exercise #{repNumber} ..."
	if File.exist?('zones/itm.log')
		File.delete('zones/itm.log')
	end
	if File.exist?('zones/txc.stats')
		File.delete('zones/txc.stats')
	end
	
	IO.popen("-", "r+") do |pipe|
		if pipe.nil?
			system('opcontrol --reset')
			system('opcontrol --start')
			sleep(4)
			startBind(Logged) do |bindIn, bindOut, bindErr|
				$stdout.puts "Started BIND!"

				# BIND will be ended by an interrupt signal from the parent:
				# Note race condition here!
				trap('INT')	{}
				$stdout.puts "Bind Terminated!"
			end
		else
			# Wait for the other thread to get BIND ready
			$stderr.puts pipe.gets

			# Run queryperf against BIND!
			IO.popen("#{BindRoot}/contrib/queryperf/queryperf " +
			         "-p #{Port} -q 400 -d #{ExperimentsRoot}/queries.dat -l 30", "r") do |queryperfStdio|
				$stderr.puts "Started queryperf!"
				$stderr.puts "Waiting for queryperf to finish..."
				queryperfStdio.readlines
			end

			# Kill BIND and synchronize such that itm.log has been created.
			killBind
			$stderr.puts pipe.gets
			
			system('opcontrol --stop > /dev/null')
			system('opcontrol --dump')

			# Copy full statistics
			until File.exist?("zones/itm.log") # and File.exist?('zones/txc.stats')
				sleep(0.5)
			end
			FileUtils::copy('zones/itm.log', ReportFile + ".itm.log.#{repNumber}")
			FileUtils::copy('zones/txc.stats', ReportFile + ".txc.stats.#{repNumber}")
			system("opreport -l -c > #{ReportFile}.oprofile.#{repNumber}")
			
			# Read Statistics
			logFile = File.open("zones/itm.log", "r")
			line = logFile.readlines

			thisEntry = Hash.new
			line.count.downto 0 do |lineNumber|
				thisLine = line[lineNumber - 1]
				$stderr.puts thisLine
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

if File.exist?('zones/txc.ini')
	File.delete('zones/txc.ini')
end

puts "#{stats.average[:transactions]} transactions, " +
	"#{stats.average[:retries]} conflict retries, " +
	"#{stats.average[:userRetries]} explicit retries, " +
	"#{stats.average[:serialTransitions] * 100}% forced serial."
