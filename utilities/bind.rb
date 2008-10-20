def startBind
	Open3.popen3("#{BindRoot}/bin/named/named -f -g -p #{Port} -c #{Configuration}") do |bindIn, bindOut, bindErr|
		while output = bindErr.gets and not output =~ /running/
			$stderr.puts output
		end
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
	bindPid
end
