$stdout.write <<EOF
// BIND configuration file

options {
	directory "#{Dir.getwd}/zones";
	pid-file "named.pid";
};

zone "movie.edu" in {
	type master;
	file "db.movie.edu";
};

zone "249.249.192.in-addr.arpa" in {
	type master;
	file "db.192.249.249";
};

zone "253.253.192.in-addr.arpa" in {
	type master;
	file "db.192.253.253";
};

zone "0.0.127.in-addr.arpa" in {
	type master;
	file "db.127.0.0";
};

zone "." in {
	type hint;
	file "db.cache";
};
EOF