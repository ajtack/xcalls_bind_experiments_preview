# Uses regular expressions to match the URLs in standard input and extract their domains.

urls = Array.new

$stdin.readlines.each do |line|
	urls += line.scan(/https?:\/\/([-\w\.]+)/)
end

urls.flatten!
urls.compact!
urls.uniq!
urls.sort.each { |url| puts url }
