$stdout.write <<EOF
// BIND configuration file

options {
	directory "#{Dir.getwd}/zones";
	pid-file "named.pid";
};

zone "." in {
	type master;
	file "db.example.com";
};
EOF

if (ARGV[0] == '--logging')
	$stdout.write <<LOGGING

logging{
  channel simple_log {
    file "bind.log" versions 3 size 5m;
    severity dynamic;
    print-time yes;
    print-severity yes;
    print-category yes;
  };
  category default{
    simple_log;
  };
};
LOGGING

end
