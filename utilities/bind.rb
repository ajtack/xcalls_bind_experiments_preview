require 'open3'

def startBind(logged=false)
	commandString = "export ITM_STATISTICS=simple; #{BindRoot}/bin/named/named -f -p #{Port} -c #{Configuration} -n #{CpuCount}"
	if logged
		commandString += ' -d 3'
	end
	
	Open3.popen3(commandString) do |bindIn, bindOut, bindErr|
		#while output = bindErr.gets and not output =~ /running/
		#	$stderr.puts output
		#end
		sleep(1)
		if block_given?
			yield bindIn, bindOut, bindErr
		end
	end
end

def killBind
	# Kill the BIND instance to collect its output
	bindPid = File.new("zones/named.pid").read.to_i
	$stderr.puts "Killing BIND @ #{bindPid}..."
	
	
	Process.kill('INT', bindPid)
	sleep(2)
	
	bindPid
end
