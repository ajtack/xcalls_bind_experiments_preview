#
# Untars and builds BIND with the correct environment for STM,
# and then runs the script to apply the set of available
# patches to BIND and run given experiments against it.
#

CC = icc
CFLAGS += \
	-Qtm_enabled \
	-DTM_CALLABLE=\"__attribute__((tm_callable))\" \
	-DTM_WAIVER=\"__attribute__((tm_pure))\" \
	
LIBS += \
	$(PWD)/xCalls/build/libtxc.a
	
CONFIGURE_OPTIONS = --enable-threads --with-openssl=no
BIND_DIRECTORY = bind-9.3.5-P2
PATCHES_DIRECTORY = patches
EXPERIMENTS_DIRECTORY = experiments

.PHONY: default_target
default_target: experiments

xCalls/build/libtxc.a:
	@echo "Building XCalls ..."
	@export CC=`which icc` && \
		scons -C xCalls linkage=static

.PHONY: experiments
experiments: named.conf $(EXPERIMENTS_DIRECTORY)/queries.dat
	@echo "Running Experiments ..."
	@ruby run.rb $(BIND_DIRECTORY) $(EXPERIMENTS_DIRECTORY) $(PATCHES_DIRECTORY)

$(EXPERIMENTS_DIRECTORY)/queries.dat: named.conf
	@echo "Generating query set ..."
	@curl -s http://www.mit.edu/people/cdemello/univ-full.html | \
		ruby utilities/scrape_domains.rb | \
		ruby utilities/generate_query_set.rb 100000 > \
		$@

zones/db.example.com: utilities/generate_example_zone.rb
	@echo "Generating a local authoritative zone ..."
	@curl -s http://www.mit.edu/people/cdemello/univ-full.html | \
		ruby utilities/scrape_domains.rb | \
		ruby utilities/generate_example_zone.rb > $@

named.conf: utilities/build_named.conf.rb zones/db.example.com
	@echo "Generating a configuration for BIND ..."
	@ruby $< --logging > $@

$(BIND_DIRECTORY).tar.gz:
	@echo "Downloading $@ ..."
	@curl -s http://ftp.isc.org/isc/bind9/9.3.5-P2/bind-9.3.5-P2.tar.gz > $@

$(BIND_DIRECTORY): $(BIND_DIRECTORY).tar.gz xCalls/build/libtxc.a
	@echo "Unarchiving Vanilla BIND ..."
	@tar xzf $<
	@echo "Configuring BIND ..."
	@cd $(BIND_DIRECTORY) && \
		export CFLAGS="$(CFLAGS)" && \
		export CC=$(CC) && \
		export LIBS="$(LIBS)" && \
		./configure $(CONFIGURE_OPTIONS) > /dev/null && \
		patch -p1 < ../include_xcalls.patch && \
		./configure $(CONFIGURE_OPTIONS) > /dev/null \

.PHONY: build_named
build_named: $(BIND_DIRECTORY) named.conf zones/db.example.com
	@echo "Building BIND..."
	make -C $(BIND_DIRECTORY)
	@echo "Building QueryPerf..."
	@cd $(BIND_DIRECTORY)/contrib/queryperf && ./configure > /dev/null
	@make -s --no-print-directory -C $(BIND_DIRECTORY)/contrib/queryperf

.PHONY: restore
restore: clear build_named

.PHONY: empty_cores
empty_cores:
	@rm -Rf zones/core.*

.PHONY: clean
clean: clear empty_cores
	@rm -f zones/itm.log
	@rm -f zones/bind.log*
	@rm -f $(EXPERIMENTS_DIRECTORY)/queries.dat
	@rm -f zones/db.example.com
	@rm -f named.conf
	scons -C xCalls -c

.PHONY: clear
clear: empty_cores
	@rm -Rf $(BIND_DIRECTORY)

.PHONY: patch
patch: make_patch.rb $(BIND_DIRECTORY)
	rm -Rf $(BIND_DIRECTORY)
	make $(BIND_DIRECTORY)
	@ruby $< '$(BIND_DIRECTORY)' '$(PATCHES_DIRECTORY)'
	@echo "Patch Creation Successful!"

