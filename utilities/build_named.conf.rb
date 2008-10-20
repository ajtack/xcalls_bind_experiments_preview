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