package j3;

import org.osgi.framework.Bundle;

public interface J3Mgr
{
	public void setBundleStaleReferenceCorrected(Bundle bundle, boolean corrected) throws Exception;
	public boolean isBundleStaleReferenceCorrected(Bundle bundle) throws Exception;
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void dumpClassLoaderBundles();
	public void setBundleStaleReferenceCorrected(String bundleName, String corrected) throws Exception;
	public void isBundleStaleReferenceCorrected(String bundleName) throws Exception;
}
