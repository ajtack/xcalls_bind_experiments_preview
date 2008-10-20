$stdout.write <<EOF
// BIND configuration file

options {
	directory "#{Dir.getwd}/zones";
	pid-file "named.pid";
};

zone "example.com." in {
	type master;
	file "db.example.com";
}

zone "." in {
	type hint;
	file "db.cache";
};
EOF