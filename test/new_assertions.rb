module NewAssertions
  def assert_redirects
    assert_response(:redirect)
  end
  def assert_blank(object)
    assert object.blank?, "#{object.inspect} is not blank"
  end
  def assert_size(size,object)
    assert_equal size, object.size,
      "#{object.inspect} was expected to be size #{size} " +
      "but was size #{object.size}"
  end
  
  def assert_body_contains(string)
    assert_match string, response.body
  end

  def assert_scope_equal(expected_hash,scope)
    assert_equal expected_hash,scope.scope(:find)
  end
end
