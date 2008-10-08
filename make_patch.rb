#!ruby
#
# This script makes a patch comparing two versions of a directory,
# writing this patch to the given location. It is here to make my
# life easier, making patches.
#
# Arguments:
VanillaDirectory = ARGV[0]  # The "unchanged code" directory.
PatchesDirectory = ARGV[1]  # The place to put the patch generated.

puts "Which directory is being compared with #{VanillaDirectory}?"
comparedDirectory = $stdin.gets.strip

puts "What to call the patch?"
patchName = $stdin.gets.strip

# Add the extension .patch to the patch name if it was
# not provided originally.
if not patchName.split('.').last == 'patch'
	patchName += '.patch'
end

system("make -C #{comparedDirectory} clean")
system("rm -f #{PatchesDirectory}/#{patchName}")
diffResult = system("diff -x 'Makefile' -x '*.i' -x '*~' -x 'make' -x '*.log' -x 'doc' -x 'contrib' -x 'tests' -x 'config.h' -x 'config.status' -x 'isc-config.h' -x '*.kdev*' -x 'platform.h' -x 'netdb.h' -x 'Doxyfile' -x 'port_*.h' --recursive #{VanillaDirectory} #{comparedDirectory} > #{PatchesDirectory}/#{patchName}")

puts diffResult
exit 0

