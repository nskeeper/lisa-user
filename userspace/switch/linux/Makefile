TOP_DIR		:= $(shell pwd)/../../
CFLAGS		+= -fPIC -g



include $(TOP_DIR)/scripts/Makefile.includes
INC		= $(TOP_DIR)/switch \
		  $(TOP_DIR)/include

# LiSA Middleware
SRCS = util_linux.c linux.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)
