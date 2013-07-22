ifdef DEBUG
	BUILD_MODE	:= Debug+Asserts
else
	BUILD_MODE	:= Release+Asserts
endif

TEMP_LEVEL	:= ../..
RUN_BENCH	:= $(PWD)/$(LEVEL)/run-bench.sh
DACAPO		:= $(PWD)/$(LEVEL)/jars/dacapo-$(DACAPO_VERSION).jar
JVM1_PATH	:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/incinerator/$(BUILD_MODE)/bin/j3
JVM2_PATH	:= $(PWD)/../$(TEMP_LEVEL)/$(LEVEL)/incinerator/$(BUILD_MODE)_NoIncinerator/bin/j3
#WORK_DIR	:= $(LEVEL)
WORK_DIR	:= $(HOME)/dacapo_benchmarks

ifndef ENABLE_JVM1
	ENABLE_JVM1 := 1
endif

ifndef ENABLE_JVM2
	ENABLE_JVM2 := 1
endif

ifndef VERBOSE
	QUIET	:= @
endif

TMP_DIR	:= $(shell mktemp -d --suffix=.$(DACAPO_VERSION) --tmpdir=$(WORK_DIR))

all: $(DECAPO_BENCHMARKS)

$(DECAPO_BENCHMARKS): $(DACAPO)
	$(QUIET) mkdir -p "$(TMP_DIR)/$@" 2>/dev/null
	$(QUIET) if [ 1 -eq $(ENABLE_JVM1) ] ; then cd "$(TMP_DIR)/$@" && "$(JVM1_PATH)" -jar "$(DACAPO)" $(DACAPO_ARGS) "$@" > "$(TMP_DIR).$@.log" 2>&1 ; else true ; fi
	$(QUIET) if [ 1 -eq $(ENABLE_JVM2) ] ; then cd "$(TMP_DIR)/$@" && "$(JVM2_PATH)" -jar "$(DACAPO)" $(DACAPO_ARGS) "$@" > "$(TMP_DIR).$@.log" 2>&1 ; else true ; fi

clean:
	@-rm -rf $(WORK_DIR)/tmp.* $(WORK_DIR)/*.log 2>/dev/null
