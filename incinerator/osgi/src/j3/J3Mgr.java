package j3;

public interface J3Mgr
{
	public void setBundleStaleReferenceCorrected(long bundleID, boolean corrected) throws Exception;
	public boolean isBundleStaleReferenceCorrected(long bundleID) throws Exception;
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void dumpClassLoaderBundles();
	public void setBundleStaleReferenceCorrected(String bundleNameOrID, String corrected) throws Exception;
	public void isBundleStaleReferenceCorrected(String bundleNameOrID) throws Exception;
}
