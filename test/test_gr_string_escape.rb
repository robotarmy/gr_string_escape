#encoding: utf-8
require 'helper'

class TestGrStringEscape < Test::Unit::TestCase
  
  STRING_ESCAPE = GrStringEscape.new
  
  def escape_text(text, cap_at = 0, add_dots = false)
    STRING_ESCAPE.parse(text.to_s, cap_at, add_dots ? '...' : '')
  end
  
  def test_gr_string_escape_default
    str = "[hi:philip]"
    assert_equal str, GrStringEscape.new.parse('[hi:philip]',str.size,"")
  end
  
  
  
  
  def test_cap_at
    test_texts = ["check for <i>italic multi-line\nblah blah</i> now normal"]
    for text in test_texts
      escape_text(text, text.length / 2, false)
    end
  end
  
  def test_unicode
    input = "日本語"
    result = escape_text(input, 1)
    assert_size 3, result
    
    input = "ひらがな"
    result = escape_text(input)
    assert_size input.size, result
    
    
    result = escape_text(input, 2)
    assert_size 6,result
  end
  
  def test_escaping_javascript
    assert(!escape_text("<script> javascript</script>").include?("<"))
    assert(!escape_text("< script>").include?("<"))
    assert(!escape_text("<script var='some other nonsense'>").include?("<"))
    assert(!escape_text('<a href="javascript:blah">testing</a>').include?("<"))
    onclick_test = escape_text('<a href="http://www.goodreads.com/test" onclick="bad">testing</a>')
    assert(!onclick_test.include?("<") || !onclick_test.include?("onclick"))
  end
  
  def test_escape_all_tags
    assert_equal("<u><strong>T</strong></u>", escape_text("<u><b>T</u></b>"))
  end
  
  def test_escaping_styles_and_classes
    assert(escape_text("<p>javascript</p>").include?("<"))
  end
  
  def test_url_highlighting
    assert(escape_text("http://www.goodreads.com/").include?("<a "))
  end
  
  def test_url_highlighting
    assert(escape_text("<b> http://www.goodreads.com/").include?("<a "))
  end
  
  def test_url_highlighting_advanced
    assert(escape_text("<p>javascript</p> http://www.goodreads.com/").include?("<a "))
  end
  
  def test_url_highlighting_advanced2
    assert(escape_text("[book:harrypotter|1]<p>javascript</p> <i> http://www.goodreads.com/").include?('href="http://www.goodreads.com/"'))
  end
  
  def test_escaping_divs
    assert(!escape_text("<div>javascript</div>").include?("<"))
  end
  
  def test_escaping_styles_and_classes2
    assert(!escape_text("<style>").include?("<"))
  end
  
  def test_escaping_styles_and_classes3
    assert(!escape_text('<p style="font-weight:bold"').include?("<"))
  end
  
  def test_escaping_styles_and_classes4
    escaped = escape_text('<a class="blah" href="javascript:blah">testing</a>')
    assert(!escaped.include?("class") || !escaped.include?("<"))
  end
  
  
  def test_escaping_nesting
    assert_equal("<p><em>TEST</em></p>", escape_text("<p><i>TEST</i></p>"))
  end
  
  
  def test_escaping_nesting2
    assert_equal("<em><em>TEST</em></em>", escape_text("<i><i>TEST</i></i>"))
  end
  
  def test_empty_url
    escape_text("<img src= / >blah")
  end
  
  def test_escaping_nesting3
    assert_equal("<em><em>TEST</em>blah <strong>test2</strong></em>",
      escape_text("<i><i>TEST</i>blah <b>test2</b></i>"))
  end
  
  
  def test_escape_all_tags
    assert_equal("<u><strong>T</strong></u>", escape_text("<u><b>T</u></b>"))
  end
end
