require 'rubygems'
require 'rake'
jeweler_tasks = nil
begin
  require 'jeweler'
  jeweler_tasks = Jeweler::Tasks.new do |gem|
    gem.name = "gr_string_escape"
    gem.summary = %Q{Goodreads string parser}
    gem.description = %Q{Code for Goodreads String Parsing}
    gem.email = "github.com@robotarmyma.de"
    gem.homepage = "http://github.com/robotarmy/gr_string_escape"
    gem.authors = ["Michael Economy","Curtis Schofield"]
    gem.extensions = FileList['ext/**/extconf.rb']
    # gem is a Gem::Specification... see http://www.rubygems.org/read/chapter/20 for additional settings
  end
  Jeweler::GemcutterTasks.new
rescue LoadError
  puts "Jeweler missing : \n  gem install jeweler"
end
begin
  require 'rake/extensiontask'
rescue LoadError  
  puts "rake-compiler missing : \n gem install rake-compiler"
end
Rake::ExtensionTask.new('gr_string_escape', jeweler_tasks.gemspec)
CLEAN.include 'lib/**/*.so'


require 'rake/testtask'
Rake::TestTask.new(:test) do |test|
  test.libs << 'lib' << 'test'
  test.pattern = 'test/**/test_*.rb'
  test.verbose = true
end

begin
  require 'rcov/rcovtask'
  Rcov::RcovTask.new do |test|
    test.libs << 'test'
    test.pattern = 'test/**/test_*.rb'
    test.verbose = true
  end
rescue LoadError
  task :rcov do
    abort "RCov is not available. In order to run rcov, you must: sudo gem install spicycode-rcov"
  end
end

task :test => :check_dependencies

task :default => :test

require 'rake/rdoctask'
Rake::RDocTask.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "gr_string_escape #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('lib/**/*.rb')
end
