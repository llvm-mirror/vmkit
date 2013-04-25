TEMP_LEVEL			:= ../..
DACAPO				:= $(PWD)/$(LEVEL)/jars/dacapo-$(DACAPO_VERSION).jar
LOG_DIR				:= $(LEVEL)/logs
LOG_DIR_FROM_TEMP	:= $(PWD)/$(LOG_DIR)
J3_PATH				:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/vmkit_stale_ref/Debug+Asserts/bin/j3
TEMP_J3				:= $(PWD)/$(LEVEL)/$(DACAPO_VERSION)/j3
IJVM_PATH			:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/ijvm/Debug+Asserts/bin/j3
TEMP_IJVM			:= $(PWD)/$(LEVEL)/$(DACAPO_VERSION)/ijvm

ifndef ENABLE_J3
	ENABLE_J3 := 1
endif

ifndef ENABLE_IJVM
	ENABLE_IJVM := 1
endif

J3_DEFINED := [ "$(ENABLE_J3)" -ne 0 ]
IJVM_DEFINED := [ "$(ENABLE_IJVM)" -ne 0 ]

all: $(VM) $(LOG_DIR) $(DECAPO_BENCHMARKS)

$(LOG_DIR):
	@mkdir -p $@ 2>/dev/null

$(DECAPO_BENCHMARKS):
	@-$(J3_DEFINED) && echo "Dacapo benchmark version " $(DACAPO_VERSION) ": j3 ==> " $@
	@-$(J3_DEFINED) && rm -rf $(TEMP_J3)/$@ && mkdir -p $(TEMP_J3)/$@
	@-$(J3_DEFINED) && cd $(TEMP_J3)/$@ && $(J3_PATH) -jar $(DACAPO) $(DACAPO_ARGS) $@ >$(LOG_DIR_FROM_TEMP)/j3-$@-$(DACAPO_VERSION).log 2>&1
	@-$(IJVM_DEFINED) && echo "Dacapo benchmark version " $(DACAPO_VERSION) ": ijvm ==> " $@
	@-$(IJVM_DEFINED) && rm -rf $(TEMP_IJVM)/$@ && mkdir -p $(TEMP_IJVM)/$@
	@-$(IJVM_DEFINED) && cd $(TEMP_IJVM)/$@ && $(IJVM_PATH) -jar $(DACAPO) $(DACAPO_ARGS) $@ >$(LOG_DIR_FROM_TEMP)/ijvm-$@-$(DACAPO_VERSION).log 2>&1

clean:
	@-rm -rf $(LOG_DIR) $(TEMP_J3) $(TEMP_IJVM) 2>/dev/null
