ifdef DEBUG
	BUILD_MODE	:= Debug+Asserts
else
	BUILD_MODE	:= Release+Asserts
endif

TEMP_LEVEL			:= ../..
RUN_BENCH			:= $(PWD)/$(LEVEL)/run-bench.sh
DACAPO				:= $(PWD)/$(LEVEL)/jars/dacapo-$(DACAPO_VERSION).jar
TMP_DIR				:= $(LEVEL)/tmp
LOG_DIR				:= $(LEVEL)/logs
J3_PATH				:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/vmkit2/$(BUILD_MODE)/bin/j3
IJVM_PATH			:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/ijvm/$(BUILD_MODE)/bin/j3

ifndef ENABLE_J3
	ENABLE_J3 := 1
endif

ifndef ENABLE_IJVM
	ENABLE_IJVM := 1
endif

J3_DEFINED := [ "$(ENABLE_J3)" -ne 0 ]
IJVM_DEFINED := [ "$(ENABLE_IJVM)" -ne 0 ]

all: $(VM) $(TMP_DIR) $(LOG_DIR) $(DECAPO_BENCHMARKS)

$(TMP_DIR) $(LOG_DIR):
	@mkdir -p $@ 2>/dev/null

$(DECAPO_BENCHMARKS): $(DACAPO)
	@$(RUN_BENCH) "$(ENABLE_J3)" "j3" "$(DACAPO_VERSION)" "$(J3_PATH)" "$(DACAPO)" "$(DACAPO_ARGS)" "$@"
	@$(RUN_BENCH) "$(ENABLE_IJVM)" "ijvm" "$(DACAPO_VERSION)" "$(IJVM_PATH)" "$(DACAPO)" "$(DACAPO_ARGS)" "$@"

clean:
	@-rm -rf "$(TMP_DIR)" "$(LOG_DIR)" 2>/dev/null
