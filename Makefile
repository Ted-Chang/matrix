# Makefile for matrix system
# This is the top level makefile of the whole system.

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be found in ./README

# non-source files that are part of the distribution
AUXFILES := README

# Define subsystems in the project.
DIRS-y = kernel sdk uspace tools

.PHONY: all clean $(DIRS-y) help

all: $(DIRS-y)
	@:

empty_rule:
	@:

clean: $(DIRS-y)
	@:

$(DIRS-y):
	$(Q)$(MAKE) -C $@ S=$S$(S:%=/)$@ $(MAKECMDGOALS) $(MAKESUBDIRFLAGS)

help:
	@echo "Available make targets:"
	@echo
	@echo "all     - build matrix"
	@echo "clean   - remove all object files, dependency files"
	@echo "help    - print this help info"
