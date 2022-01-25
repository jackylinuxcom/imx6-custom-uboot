BUILD_PLAT	:=	$(srctree)
all: msg_start
msg_start:
	@echo "Building ${BUILD_PLAT}/acs.bin"

# Without the below flag the resulting acs.bin fails
LDFLAGS		+=	--gc-sections

SOURCES		+=	acs.c acs_entry.S
LINKERFILE_T	:=	acs.ld.S

ifeq (${GIT_VERSION},)
	GIT_VERSION := $(shell git describe --abbrev=7 --dirty --always --tags)
endif
ifeq (${BUILDER},)
	BUILDER := ${USER}@$(shell hostname)
endif

define MAKE_C
$(eval OBJ := $(1)/$(patsubst %.c,%.o,$(notdir $(2))))
$(OBJ) : $(2)
	@echo "  CC      $$<"
	$$(Q)$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

define MAKE_S
$(eval OBJ := $(1)/$(patsubst %.S,%.o,$(notdir $(2))))
$(OBJ) : $(2)
	@echo "  CC      $$<"
	$$(Q)$$(CC) $$(ASFLAGS) -c $$< -o $$@
endef

define MAKE_LD
$(1) : $(2)
	@echo "  PP      $$<"
	$$(Q)$$(CC) $$(ASFLAGS) -P -E -o $$@ $$<
endef

define MAKE_OBJS
	$(eval C_OBJS := $(filter %.c,$(2)))
	$(eval REMAIN := $(filter-out %.c,$(2)))
	$(eval $(foreach obj,$(C_OBJS),$(call MAKE_C,$(1),$(obj))))
	$(eval S_OBJS := $(filter %.S,$(REMAIN)))
	$(eval REMAIN := $(filter-out %.S,$(REMAIN)))
	$(eval $(foreach obj,$(S_OBJS),$(call MAKE_S,$(1),$(obj))))
	$(and $(REMAIN),$(error Unexpected source files present: $(REMAIN)))
endef

define SOURCES_TO_OBJS
	$(notdir $(patsubst %.c,%.o,$(filter %.c,$(1)))) \
	$(notdir $(patsubst %.S,%.o,$(filter %.S,$(1))))
endef

define MAKE_BL
	$(eval BUILD_DIR  := ${BUILD_PLAT})
	$(eval SOURCES    := $(SOURCES))
	$(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))
	$(eval LINKERFILE := $(BUILD_DIR)/acs.ld)
	$(eval MAPFILE    := $(BUILD_DIR)/acs.map)
	$(eval ELF        := $(BUILD_DIR)/acs.elf)
	$(eval DUMP       := $(BUILD_DIR)/acs.dump)
	$(eval BIN        := $(BUILD_DIR)/acs.bin)
	$(eval $(call MAKE_OBJS,$(BUILD_DIR),$(SOURCES)))
	$(eval $(call MAKE_LD,$(LINKERFILE),$(LINKERFILE_T)))

$(ELF) : $(OBJS) $(LINKERFILE)
	@echo "  LD      $$@"
	@echo 'const char build_message[] = "Built : "__TIME__", "__DATE__". \\nver:${GIT_VERSION} - ${BUILDER}";' \
		> $(BUILD_DIR)/build_message.c
	$$(Q)$$(CC) $$(CFLAGS) -c $(BUILD_DIR)/build_message.c -o $(BUILD_DIR)/build_message.o
	$$(Q)$$(LD) -o $$@ $$(LDFLAGS) -Map=$(MAPFILE) --script $(LINKERFILE) \
					$(BUILD_DIR)/build_message.o $(OBJS)

$(DUMP) : $(ELF)
	@echo "  OD      $$@"
	$${Q}$${OBJDUMP} -dxS $$< > $$@

$(BIN) : $(ELF)
	@echo "  BIN     $$@"
	$$(Q)$$(OBJCOPY) -O binary $$< $$@
	@echo
	@echo "Built $$@ successfully"
	@echo

.PHONY : bl$(1)
bl$(1) : $(BUILD_DIR) $(DUMP) $(BIN)

all : bl$(1)

$(eval FIP_DEPS += $(if $2,$(BIN),))
$(eval FIP_ARGS += $(if $2,--bl$(1) $(BIN),))
endef

$(eval $(call MAKE_BL,2,in_fip))
