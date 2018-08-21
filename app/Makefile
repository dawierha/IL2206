# @file: Makefile
# @author: George Ungureanu, KTH/EECS/ELE
# @date: 20.08.2018
# @version: 0.1
#
# This file automates the compilation and deployment of lab
# applications. At the beginning of each run it loads a static
# configuration from Makefile.conf which defines a particular
# application. The application can be changed by running the command
# 'make conf' (check 'make help' for instructions on usage).

# Function that checks if a list of variables has been properly
# loaded. You can ignore this part.
check_defined = $(foreach 1, $1, $(call $(if $(value $1),, \
	$(error Undefined variable $1. Make sure it is defined in the application configuration file))))

# imports the current path of the configuration file. Defines the
# $(CFG) variable.
include Makefile.conf

# The project-specific configuration file should contain the
# definitions for the variables: $(APP_NAME), $(CORE_FILE),
# $(SOF_FILE), $(JDI_FILE), $(BSP_PATH), $(CPU_NAME) $(BSP_TYPE)
# $(NIOS2_BSP_COMMANDS) and $(MAKEFILE_COMMANDS).

ifneq ("$(wildcard $(CFG))","")  # if $(CFG) points to an existing file
include $(CFG)
$(call check_defined, APP_NAME CORE_FILE SOF_FILE JDI_FILE BSP_PATH CPU_NAME BSP_TYPE NIOS2_BSP_COMMANDS MAKEFILE_COMMANDS)
endif


ELF_FILE  := $(APP_NAME)/$(APP_NAME).elf
BSP_FILE  := $(BSP_PATH)/settings.bsp
MAKE_FILE := $(APP_NAME)/Makefile

# Default rule. Checks if an executable binary has been compiled by
# visiting the rule associated with its only prerequisite (see rules
# below).
compile: $(ELF_FILE)

# Rule for deploying and executing the application on the DE2
# board. It checks first if the binary is compiled, and if not it
# executes the rule associated with its prerequisite (see rules
# below).
run: $(ELF_FILE)
	nios2-configure-sof $(SOF_FILE)
	@echo "Starting a new Nios II terminal..."
	(xterm -title "$(CPU_NAME)" -e "nios2-terminal -i 0" &)
	nios2-download -g $(ELF_FILE) --cpu_name $(CPU_NAME) --jdi $(JDI_FILE)

# Rule which states that the target in the rhs is dependent on the
# prerequisites in the lhs. In case the rhs does not exist or the lhs
# is modified, then the commands below are executed, otherwise they
# are ignored.
#
# This rule builds the BSP library and generates the makefile by
# visiting the targets associated with each prerequisite; compiles the
# C application into an executable binary by executing the 'make'
# command on the generated makefile.
$(ELF_FILE) : $(BSP_FILE) $(MAKE_FILE)
	$(MAKE) -C $(APP_NAME)

# This rule invokes the command 'nios2-bsp'. Check documentation for
# argument description.
#
# Note: $@ returns the target (rhs)
#       $< returns the first prerequisite (lhs)
#       $(@D) returns the parent directory of the rhs
$(BSP_FILE) : $(CORE_FILE) $(CFG)
	nios2-bsp $(BSP_TYPE) $(@D) $< \
		--cpu-name $(CPU_NAME) \
		$(NIOS2_BSP_COMMANDS)

# This rule invokes the command 'nios2-app-generate-makefile' from
# within the $(APP_NAME) directory. Check documentation for argument
# description.
#
# Note: $(<D) returns the parent directory of the first prerequisite
$(MAKE_FILE) : $(BSP_FILE) $(CFG)
	cd $(APP_NAME) \
	&& nios2-app-generate-makefile \
		--bsp-dir ../$(<D) \
		--elf-name $(APP_NAME).elf \
		--src-dir $(SRC_PATH) \
		$(MAKEFILE_COMMANDS)

# Command for configuring this Makefile to compile and run a different
# application by updating the Makefile.conf file. It needs to be
# called with the CFG variable set to point to the new application
# configuration file.
config :
	@echo "CFG := $(CFG)" > Makefile.conf
	@echo "* Configured 'make' to load $(CFG)\n* Run the 'make' command again."
	exit 0


# cleans all generated file for the current application.
clean:
	$(MAKE) -C $(APP_NAME) clean
	rm -rf $(BSP_PATH)
	rm -f $(MAKE_FILE)

# Shows the current target application.
show:
	@echo $(APP_NAME)

# prints a help message with the main rules and their usage.
help:
	@echo "usage: make [rule] [VARIABLE=value]"
	@echo "Rules:"
	@echo "  compile : default rule. compiles an application downto binary."
	@echo "  run     : downloads and runs an application on the DE2 board."
	@echo "  config  : configures the current make script to run a different"
	@echo "            application. It requires the variable 'CFG' to point to"
	@echo "            the configuration file of the new application."
	@echo "  clean   : cleans the generated files for the current application."
	@echo "  show    : prints the current application."
	@echo "  help    : prints this help message."

.PHONY: config clean compile run show help

# Include auto-generated dependencies
-include *.d
