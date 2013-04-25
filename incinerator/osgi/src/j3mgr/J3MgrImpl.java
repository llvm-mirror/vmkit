package j3mgr;

import j3.J3Mgr;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.BundleEvent;
import org.osgi.framework.BundleListener;
import org.osgi.framework.ServiceReference;
import org.osgi.service.packageadmin.PackageAdmin;

public class J3MgrImpl
	implements J3Mgr, BundleListener
{
	BundleContext context;
	
	public void open(BundleContext bundleContext)
	{
		context = bundleContext;
		
		context.addBundleListener(this);
	}

	public void close()
	{
		context.removeBundleListener(this);
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

	public void bundleChanged(BundleEvent event)
	{
		if (event.getType() != BundleEvent.UNINSTALLED) return;
		j3.vm.OSGi.notifyBundleUninstalled(event.getBundle().getBundleId());
		refreshFramework();
	}
	
	void refreshFramework()
	{
		ServiceReference<?> pkgAdminRef = (ServiceReference<?>)context.getServiceReference(
			"org.osgi.service.packageadmin.PackageAdmin");
		PackageAdmin pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
		pkgAdmin.refreshPackages(null);
		context.ungetService(pkgAdminRef);
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
