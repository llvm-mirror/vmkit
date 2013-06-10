ifdef DEBUG
	BUILD_MODE	:= Debug+Asserts
else
	BUILD_MODE	:= Release+Asserts
endif

TEMP_LEVEL	:= ../..
RUN_BENCH	:= $(PWD)/$(LEVEL)/run-bench.sh
DACAPO		:= $(PWD)/$(LEVEL)/jars/dacapo-$(DACAPO_VERSION).jar
TMP_DIR		:= $(LEVEL)/tmp
LOG_DIR		:= $(LEVEL)/logs
JVM1_PATH	:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/incinerator/$(BUILD_MODE)_NoIncinerator/bin/j3
JVM2_PATH	:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/incinerator/$(BUILD_MODE)/bin/j3

ifndef ENABLE_JVM1
	ENABLE_JVM1 := 1
endif

ifndef ENABLE_JVM2
	ENABLE_JVM2 := 1
endif

all: $(VM) $(DECAPO_BENCHMARKS)

$(DECAPO_BENCHMARKS): $(DACAPO)
	@-$(RUN_BENCH) "$(ENABLE_JVM1)" "jvm1" "$(DACAPO_VERSION)" "$(JVM1_PATH)" "$(DACAPO)" "$(DACAPO_ARGS)" "$@"
	@-$(RUN_BENCH) "$(ENABLE_JVM2)" "jvm2" "$(DACAPO_VERSION)" "$(JVM2_PATH)" "$(DACAPO)" "$(DACAPO_ARGS)" "$@"

clean:
	@-rm -rf "$(TMP_DIR)" "$(LOG_DIR)" 2>/dev/null
