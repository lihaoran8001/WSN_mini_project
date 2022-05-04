# CONTIKI_PROJECT = nullnet-unicast nullnet-broadcast
# all: $(CONTIKI_PROJECT)

# CONTIKI = ../..
# DEFINES+=PROJECT_CONFIG=\"project-conf.h\"
# PLATFORMS_EXCLUDE = nrf52dk
# TARGET_LIBFILES += -lm

# #use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
# MAKE_MAC ?= MAKE_MAC_CSMA
# MAKE_NET = MAKE_NET_NULLNET
# include $(CONTIKI)/Makefile.include


CONTIKI_PROJECT = channel_sensing
all: $(CONTIKI_PROJECT)
DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
MAKE_NET = MAKE_NET_NULLNET
TARGET_LIBFILES += -lm
CONTIKI = ../..
include $(CONTIKI)/Makefile.include