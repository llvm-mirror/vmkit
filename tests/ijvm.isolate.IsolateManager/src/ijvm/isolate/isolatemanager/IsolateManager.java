package ijvm.isolate.isolatemanager;

import org.osgi.framework.Bundle;
import org.osgi.framework.Version;

public interface IsolateManager
{
	public void killBundles(Bundle[] bundles) throws Exception;
	public void killBundles(String bundleName1, String bundleName2) throws Exception;
	public void killBundle(String bundleName) throws Exception;
}
