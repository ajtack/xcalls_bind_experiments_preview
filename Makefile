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

.PHONY: default_target
default_target: experiments

.PHONY: experiments
experiments: named.conf experiments/queries.dat
	@echo "Running Experiments..."
	ruby run.rb $(BIND_DIRECTORY) experiments patches

experiments/queries.dat:
	ruby experiments/generate_query_set.rb 20000 > $@

named.conf: build_named.conf.rb
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

.PHONY: clean
clean:
	make -C $(BIND_DIRECTORY) clean

.PHONY: clear
clear:
	rm -Rf $(BIND_DIRECTORY)

.PHONY: patch
patch: make_patch.rb $(BIND_DIRECTORY) clean
	ruby $< '$(BIND_DIRECTORY)' '$(PATCHES_DIRECTORY)'
	@echo "Patch Creation Successful!"

