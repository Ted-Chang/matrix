# Makefile for matrix system
# This is the top level makefile of the whole system.

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be found in ./README

# non-source files that are part of the distribution
AUXFILES := README

# Define subsystems in the project.
SUBSYS := kernel sdk uspace tools

.PHONY: all clean help

all: $(SUBSYS)
	@echo "system build done."

$(SUBSYS): Makefile
	for d in $(SUBSYS); do (cd $$d; $(MAKE)); done;

clean:
	for d in $(SUBSYS); do (cd $$d; $(MAKE) clean); done;

help:
	@echo "Available make targets:"
	@echo
	@echo "all     - build matrix"
	@echo "clean   - remove all object files, dependency files"
	@echo "help    - print this help info"
