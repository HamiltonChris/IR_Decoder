# add @ for silent output
SILENCE = @

#---- Outputs ----#
COMPONENT_NAME = IR_Receiver
TARGET_LIB = \
	lib/lib$(COMPONENT_NAME).a

TEST_TARGET = \
	$(COMPONENT_NAME)_tests

#---- Inputs ----#
PROJECT_HOME_DIR = .
CPP_PLATFORM = Gcc

SRC_DIRS = \
	$(PROJECT_HOME_DIR)/src \

TEST_SRC_DIRS = \
	tests \
	tests/* \

INCLUDE_DIRS = \
	. \
	$(CPPUTEST_HOME)/include \
	$(PROJECT_HOME_DIR)/include \

CPPUTEST_WARNINGFLAGS += -Wall -Werror -Wswitch-default -Wswitch-enum


include $(CPPUTEST_HOME)/build/MakefileWorker.mk
