package j3;

public interface J3Mgr
{
	public void setBundleStaleReferenceCorrected(
		long bundleID, boolean corrected) throws Throwable;
	public boolean isBundleStaleReferenceCorrected(
		long bundleID) throws Throwable;
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void dumpClassLoaderBundles() throws Throwable;
	public void dumpReferencesToObject(String obj_address) throws Throwable;
	public void setBundleStaleReferenceCorrected(
		String bundleNameOrID, String corrected) throws Throwable;
	public void isBundleStaleReferenceCorrected(
		String bundleNameOrID) throws Throwable;
	public void forceStaleReferenceScanning() throws Throwable;
	public void forceSoftReferencesCollection();
}
