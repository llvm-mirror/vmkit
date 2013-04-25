package ijvm.isolate.vm;

import java.util.ArrayList;

public class IJVM
{
	public static native int getObjectIsolateID(Object object);
	public static native void disableIsolates(long[] isolateID, boolean denyIsolateExecution, boolean resetIsolateReferences);
}
