require 'digest'
puts "source=("
ARGV.each {|f| puts "  '#{f}'"}
puts ")"
puts "sha256sums=("
ARGV.each {|f| h = Digest::SHA256.file(f); puts "  '#{h}'"}
puts ")"
