#
# Untars and builds BIND with the correct environment for STM,
# and then runs the script to apply the power set of available
# patches to BIND and run given experiments against it.
#

CC = icc
CFLAGS = -Qtm_enabled
CONFIGURE_OPTIONS = --enable-threads --with-openssl=no
BIND_DIRECTORY = bind-9.3.5-P2
PATCHES_DIRECTORY = patches
EXPERIMENTS_DIRECTORY = experiments

.PHONY: default_target
default_target: experiments

.PHONY: experiments
experiments: named.conf experiments/queries.dat
	@echo "Running Experiments..."
	ruby run.rb $(BIND_DIRECTORY) $(EXPERIMENTS_DIRECTORY) $(PATCHES_DIRECTORY)

$(EXPERIMENTS_DIRECTORY)/queries.dat: named.conf build_named
	curl http://www.mit.edu/people/cdemello/univ-full.html | \
		ruby utilities/scrape_domains.rb | \
		ruby utilities/clean_domains.rb $(BIND_DIRECTORY) $(EXPERIMENTS_DIRECTORY) 3000 named.conf | \
		ruby utilities/generate_query_set.rb 100000 > \
		$@

zones/db.example.com: utilities/generate_example_zone.rb
	curl http://www.mit.edu/people/cdemello/univ-full.html | \
		ruby utilities/scrape_domains.rb | \
		ruby utilities/generate_example_zone.rb > $@

named.conf: utilities/build_named.conf.rb zones/db.example.com
	ruby $< > $@

$(BIND_DIRECTORY).tar.gz:
	@echo "Downloading $@"
	curl http://ftp.isc.org/isc/bind9/9.3.5-P2/bind-9.3.5-P2.tar.gz > $@

$(BIND_DIRECTORY): $(BIND_DIRECTORY).tar.gz
	@echo "Unarchiving Vanilla BIND..."
	tar xzf $<
	@echo "Configuring BIND..."
	cd $(BIND_DIRECTORY) && \
	export CFLAGS="$(CFLAGS)" && \
	export CC=$(CC) && \
	./configure $(CONFIGURE_OPTIONS)

.PHONY: build_named
build_named: $(BIND_DIRECTORY)
	@echo "Building BIND..."
	make -C $(BIND_DIRECTORY)
	@echo "Building QueryPerf..."
	cd $(BIND_DIRECTORY)/contrib/queryperf && ./configure
	make -C $(BIND_DIRECTORY)/contrib/queryperf

.PHONY: restore
restore: clear $(BIND_DIRECTORY)

.PHONY: empty_cores
empty_cores:
	rm -Rf zones/core.*

.PHONY: clean
clean: empty_cores
	make -C $(BIND_DIRECTORY) clean

.PHONY: clear
clear: empty_cores
	rm -Rf $(BIND_DIRECTORY)
	rm -f $(BIND_DIRECTORY).tar.gz
	rm -f zones/itm.log
	rm -f named.conf
	rm -f $(EXPERIMENTS_DIRECTORY)/queries.dat

.PHONY: patch
patch: make_patch.rb $(BIND_DIRECTORY) clean
	ruby $< '$(BIND_DIRECTORY)' '$(PATCHES_DIRECTORY)'
	@echo "Patch Creation Successful!"

