require 'rubygems'
require 'test/unit'

$LOAD_PATH.unshift(File.dirname(__FILE__))
$LOAD_PATH.unshift(File.join(File.dirname(__FILE__), '..', 'lib'))
require 'gr_string_escape'
require 'new_assertions'
class Test::Unit::TestCase
  include NewAssertions
end


