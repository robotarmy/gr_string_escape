require 'rubygems'
require 'test/unit'

$LOAD_PATH.unshift(File.dirname(__FILE__))
$LOAD_PATH.unshift(File.join(File.dirname(__FILE__), '..', 'lib'))
require 'gr_string_escape'

class Test::Unit::TestCase
  
  def assert_size(object, size)
    assert object.size == size,
      "#{object.inspect} was expected to be size #{size} " +
      "but was size #{object.size}"
  end
end


