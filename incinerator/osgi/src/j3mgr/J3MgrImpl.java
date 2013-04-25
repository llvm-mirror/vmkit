package j3mgr;

import j3.J3Mgr;

import org.osgi.framework.AllServiceListener;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.BundleEvent;
import org.osgi.framework.BundleListener;
import org.osgi.framework.ServiceEvent;

public class J3MgrImpl
	implements J3Mgr, BundleListener, AllServiceListener
{
	BundleContext context;
	
	public void open(BundleContext bundleContext)
	{
		context = bundleContext;
		
		context.addBundleListener(this);
		context.addServiceListener(this);
	}

	public void close()
	{
		context.removeBundleListener(this);
		context.removeServiceListener(this);
		context = null;
	}

	public void setBundleStaleReferenceCorrected(Bundle bundle, boolean corrected) throws Exception
	{
		j3.vm.OSGi.setBundleStaleReferenceCorrected(bundle.getBundleId(), corrected);
	}

	public boolean isBundleStaleReferenceCorrected(Bundle bundle) throws Exception
	{
		return j3.vm.OSGi.isBundleStaleReferenceCorrected(bundle.getBundleId());
	}

	public void serviceChanged(ServiceEvent event)
	{
		if (event.getType() != ServiceEvent.UNREGISTERING) return;
		// WARNING: Service is NOT yet unregistered here. Find a solution to know when it is.
		j3.vm.OSGi.notifyServiceUnregistered(
			event.getServiceReference().getBundle().getBundleId(),
			context.getService(event.getServiceReference()).getClass()
			);
	}

	public void bundleChanged(BundleEvent event)
	{
		if (event.getType() != BundleEvent.UNINSTALLED) return;
		j3.vm.OSGi.notifyBundleUninstalled(event.getBundle().getBundleId());
	}
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void setBundleStaleReferenceCorrected(String bundleName, String corrected) throws Exception
	{
		setBundleStaleReferenceCorrected(getBundle(bundleName), corrected.equals("yes"));
	}
	
	public void isBundleStaleReferenceCorrected(String bundleName) throws Exception
	{
		boolean value = isBundleStaleReferenceCorrected(getBundle(bundleName));
		System.out.println("isBundleStaleReferenceCorrected(" + bundleName + ") = " + (value ? "yes" : "no"));
	}

	Bundle getBundle(String symbolicName)
	{
		Bundle[] bundles = context.getBundles();
		for (int i=0; i < bundles.length; ++i) {
			if (symbolicName.equals(bundles[i].getSymbolicName()))
				return bundles[i];
		}
		return null;
	}

	public void dumpClassLoaderBundles()
	{
		j3.vm.OSGi.dumpClassLoaderBundles();
	}
}
