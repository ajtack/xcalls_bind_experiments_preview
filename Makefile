#
# Untars and builds BIND with the correct environment for STM,
# and then runs the script to apply the power set of available
# patches to BIND and run given experiments against it.
#

CC = icc
CFLAGS = -DISC_MUTEX_PROFILE=1
CONFIGURE_OPTIONS = --enable-threads
BIND_DIRECTORY = bind-9.3.5-P2
PATCHES_DIRECTORY = patches

.PHONY: default_target
default_target: experiments

.PHONY: experiments
experiments: named.conf
	@echo "Running Experiments..."
	ruby run.rb $(BIND_DIRECTORY) experiments patches

named.conf: build_named.conf.rb
	ruby $< > $@

$(BIND_DIRECTORY).tar.gz:
	@echo "Downloading $@"
	@curl http://ftp.isc.org/isc/bind9/9.3.5-P2/bind-9.3.5-P2.tar.gz > $@

$(BIND_DIRECTORY): $(BIND_DIRECTORY).tar.gz
	@echo "Unarchiving Vanilla BIND..."
	@tar xzf $<

$(BIND_DIRECTORY)/Makefile: $(BIND_DIRECTORY)
	@echo "Configuring BIND..."
	@cd $(BIND_DIRECTORY) && \
	export CC=$(CC) && \
	./configure $(CONFIGURE_OPTIONS)

.PHONY: build_named
build_named: $(BIND_DIRECTORY)/Makefile
	@echo "Building BIND..."
	@export CFLAGS="$(CFLAGS)" && \
	 make -s -C $(BIND_DIRECTORY)
	@echo "Building QueryPerf..."
	@cd $(BIND_DIRECTORY)/contrib/queryperf && ./configure
	@make -s -C $(BIND_DIRECTORY)/contrib/queryperf

.PHONY: restore
restore: clear $(BIND_DIRECTORY)/Makefile

.PHONY: clean
clean:
	@make -C $(BIND_DIRECTORY) clean

.PHONY: clear
clear:
	@rm -Rf $(BIND_DIRECTORY)
	@rm -f $(BIND_DIRECTORY).tar.gz

.PHONY: patch
patch: make_patch.rb $(BIND_DIRECTORY) clean
	ruby $< '$(BIND_DIRECTORY)' '$(PATCHES_DIRECTORY)'
	@echo "Patch Creation Successful!"

