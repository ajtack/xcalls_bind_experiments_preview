#
# Runs each ruby scripts in the 'experiments' subdirectory with the
# same set of arguments once for each patch onto BIND in the 
# 'patches' subdirectory.
#
# Arguments
BindDir        = if ARGV[0][-1..-1] == '/'
                 then ARGV[0][0..-2]
		           else ARGV[0] end
ExperimentsDir = if ARGV[1][-1..-1] == '/'
                 then ARGV[1][0..-2] 
		           else ARGV[1] end
PatchesDir     = if ARGV[2][-1..-1] == '/'
                 then ARGV[2][0..-2]
		           else ARGV[2] end

Dir.open(PatchesDir) do |PatchDir|
	patches = PatchDir.collect do |fileName|
		if fileName.split('.').last == 'patch'
			fileName
		else
			nil
		end
	end
	patches.compact!

	results = Hash.new
	patches.each do |patchName|
		results[patchName] = Hash.new

		# Restore vanilla BIND
		system("make restore")
		
		puts "Applying patch #{patchName}"
		IO.popen("patch -p2 -d #{BindDir}", mode="r+") do |patchStdio|
			patchStdio.write(IO.read(PatchesDir + '/' + patchName))
		end

		if system("make -s build_named")
			Dir.open(ExperimentsDir) do |experimentDir|
				experiments = experimentDir.collect do |fileName|
					if fileName.split('.')[-2] == 'experiment'
						fileName
					else
						nil
					end
				end
				experiments.compact!

				experiments.each do |experimentName|
					experimentName = ExperimentsDir + '/' + experimentName
					IO.popen("ruby #{experimentName} #{BindDir} #{ExperimentsDir} 3000 named.conf 10") do |experimentOutput|
						results[patchName][experimentName] = experimentOutput.read.to_i
					end
				end
			end
		else
			raise "make failed with error #{$?}!"
		end
	end

	results.each do |patchName, experiments|
		puts "Results for #{patchName}:"
		experiments.each do |experimentName, result|
			printf("\t%30s\t%10f\n", experimentName, result+ 0.5)
		end
	end
end

