package j3mgr;

import j3.J3Mgr;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class Activator
	implements BundleActivator
{
	J3MgrImpl mgr;

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("J3MgrImpl exports and provides J3Mgr");

		mgr = new J3MgrImpl();
		mgr.open(bundleContext);
		
		bundleContext.registerService(J3Mgr.class.getName(), mgr, null);
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("J3MgrImpl no more provides J3Mgr");

		if (mgr != null) {
			mgr.close();
			mgr = null;
		}
	}
}
