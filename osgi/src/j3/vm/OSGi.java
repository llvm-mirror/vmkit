package j3.vm;

public class OSGi
{
	public static native void associateBundleClass(long bundleID, Class classObject);
	public static native void resetReferencesToBundle(long bundleID);
}
