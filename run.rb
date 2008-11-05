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

require 'fileutils'

NumberOfCpusToUse = [7, 3]

NumberOfCpusToUse.each do |cpuCount|
	Dir.open(PatchesDir) do |PatchDir|
		patches = PatchDir.collect do |fileName|
			if fileName.split('.').last == 'patch' and not fileName[0,1] == '.'
				fileName
			else
				nil
			end
		end
		patches.compact!

		# Restore vanilla BIND, run patches.
		system("make --no-print-directory restore")
		results = Hash.new
		lastPatchWasLogged = false
		patches.sort.each do |patchName|
			results[patchName] = Hash.new

			buildCommand = 'make -s --no-print-directory build_named'
			isLoggedPatch = (patchName =~ /.+\.logged\.patch/)
		
			# Apply patch
			system("patch -p1 -d #{BindDir} < #{PatchesDir + '/' + patchName}")

			if system(buildCommand)
				Dir.open(ExperimentsDir) do |experimentDir|
					experiments = experimentDir.collect do |fileName|
						if fileName =~ /.+\.experiment\.rb/ and not fileName[0,1] == '.'
							fileName
						else
							nil
						end
					end
					experiments.compact!

					puts "Running against patch #{patchName} ..."
					experiments.each do |experimentName|
						experimentName = ExperimentsDir + '/' + experimentName
						FileUtils::mkdir_p("results/#{cpuCount}_cpus/#{experimentName}")
						experimentCommand = "ruby #{experimentName} #{BindDir} #{ExperimentsDir} 3000 named.conf 1 #{cpuCount} results/#{cpuCount}_cpus/#{experimentName}/#{patchName}"
						if isLoggedPatch
							experimentCommand += ' --logged'
						end
						IO.popen(experimentCommand) do |experimentOutput|
							results[patchName][experimentName] = experimentOutput.read
						end
					end
				end
			else
				raise "make failed with error #{$?}!"
			end
		
			system("patch --reverse -p1 -d #{BindDir} < #{PatchesDir + '/' + patchName}")
		end

		puts "Results for #{cpuCount} CPUs:"
		results.sort.each do |pair|
			puts "Results for #{pair[0]}:"
			pair[1].each do |experimentName, result|
				printf("\t%30s\t%s\n", experimentName.match(/#{ExperimentsDir}\/(.+)\.experiment\.rb/)[1], result.strip)
			end
		end
	end
end
