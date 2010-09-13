require 'helper'

class TestGrStringEscape < Test::Unit::TestCase
  def test_gr_string_escape_default
    str = "[hi:philip]"
    assert_equal str,GrStringEscape.new.parse('[hi:philip]',str.size,"")
  end
end
