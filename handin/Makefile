CONTIKI_PROJECT = nullnet-unicast nullnet-broadcast
MODULES += os/services/simple-energest
DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
# PFLAGS+=-DCC2420_DEF_RFPOWER = 31 / / Maximum Power
# PFLAGS+=-DCC2420_DEF_RFPOWER = 1 / / Minimum Power
all: $(CONTIKI_PROJECT)

CONTIKI = ../..

PLATFORMS_EXCLUDE = nrf52dk

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_NULLNET
include $(CONTIKI)/Makefile.include
