#include "j3/j3jni.h"
#include "j3/j3.h"
#include "j3/j3thread.h"

namespace j3 {
	class PerfData {
	public:
    // the Variability enum must be kept in synchronization with the
    // the com.sun.hotspot.perfdata.Variability class
    enum Variability {
      V_Constant = 1,
      V_Monotonic = 2,
      V_Variable = 3,
      V_last = V_Variable
    };

    // the Units enum must be kept in synchronization with the
    // the com.sun.hotspot.perfdata.Units class
    enum Units {
      U_None = 1,
      U_Bytes = 2,
      U_Ticks = 3,
      U_Events = 4,
      U_String = 5,
      U_Hertz = 6,
      U_Last = U_Hertz
    };

	private:
		const vmkit::Name* name;
		Variability        variability;
		Units              units;
		uint64_t           value;

	public:
		PerfData(const vmkit::Name* _name, Variability _variability, Units _units, uint64_t _value) {
			name = _name;
			variability = _variability;
			units = _units;
			value = _value;
		}

		uint64_t* address() { return &value; }
	};

	class PerfDataManager {
		pthread_mutex_t                   mutex;
		std::map<const vmkit::Name*, PerfData*> perfDatas;
	public:
		static PerfDataManager manager;

		PerfDataManager() {
			pthread_mutex_init(&mutex, 0);
		}

		PerfData* define(const vmkit::Name* name, PerfData::Variability variability, PerfData::Units units, uint64_t value) {
			pthread_mutex_lock(&mutex);
			PerfData* res = perfDatas[name];
			if(res)
				J3::illegalArgumentException("trying to redefine a perf counter");
			perfDatas[name] = res = new PerfData(name, variability, units, value);
			pthread_mutex_unlock(&mutex);
			return res;
		}
	};

	PerfDataManager PerfDataManager::manager;
}

using namespace j3;

extern "C" {
	JNIEXPORT void JNICALL Java_sun_misc_Perf_registerNatives(JNIEnv* env, jclass clazz) {
		// Nothing, we define the Perf methods with the expected signatures.
	}

	JNIEXPORT jobject JNICALL Java_sun_misc_Perf_createLong(JNIEnv* env, jobject perf, jstring name, 
																													jint variability, jint units, jlong value) {
		PerfData* pd;
		enterJVM();
		J3* vm = J3Thread::get()->vm();
		pd = PerfDataManager::manager.define(vm->stringToName(name), 
																									 (PerfData::Variability)variability, (PerfData::Units)units, value);
		leaveJVM();
    return env->NewDirectByteBuffer(pd->address(), sizeof(uint64_t));
	}
}
