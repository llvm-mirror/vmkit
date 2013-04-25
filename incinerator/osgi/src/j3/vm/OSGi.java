package j3.vm;

public class OSGi
{
	// OSGi hooks and information gathering
	public static native void associateBundleClass(long bundleID, Class classObject);
	public static native void notifyBundleUninstalled(long bundleID);
	
	// Commands
	public static native void resetReferencesToBundle(long bundleID);
	public static native void dumpClassLoaderBundles();
}
