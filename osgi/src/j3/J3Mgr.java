package j3;

import org.osgi.framework.Bundle;

public interface J3Mgr
{
	public void resetReferencesToBundle(Bundle bundle) throws Exception;
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void resetReferencesToBundle(String bundleName) throws Exception;
}
