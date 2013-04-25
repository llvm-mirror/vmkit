package j3mgr;

import java.util.LinkedList;

import j3.J3Mgr;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.FrameworkEvent;
import org.osgi.framework.FrameworkListener;
import org.osgi.framework.ServiceReference;
import org.osgi.service.packageadmin.PackageAdmin;

public class J3MgrImpl
	implements J3Mgr, FrameworkListener
{
	BundleContext context;
	LinkedList<Long> bundlesToKill;
	
	public void open(BundleContext bundleContext)
	{
		context = bundleContext;
		
		bundlesToKill = new LinkedList<Long>();
		
		// We need to know when packages are refreshed
		context.addFrameworkListener(this);
	}

	public void close()
	{
		context.removeFrameworkListener(this);
		context = null;
		
		bundlesToKill = null;
	}

	public void resetReferencesToBundle(Bundle bundle) throws Exception
	{
		System.out.println("resetReferencesToBundle: #" + bundle.getBundleId() + " " + bundle.getSymbolicName());
		
		System.out.println("Stopping bundle: " + bundle.getSymbolicName());
		bundle.stop();
		System.out.println("Uninstalling bundle: " + bundle.getSymbolicName());
		bundle.uninstall();

		bundlesToKill.addFirst(new Long(bundle.getBundleId()));
		
		System.out.println("Refreshing framework...");
		refreshFramework(bundle);
	}
	
	void refreshFramework(Bundle bundle)
	{
		ServiceReference<?> pkgAdminRef = (ServiceReference<?>)context.getServiceReference(
			"org.osgi.service.packageadmin.PackageAdmin");
		PackageAdmin pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
		pkgAdmin.refreshPackages(null);
	}

	public void frameworkEvent(FrameworkEvent event)
	{
		if (event.getType() != FrameworkEvent.PACKAGES_REFRESHED) return;

		long bundleID = bundlesToKill.removeLast().longValue();
		
		System.out.println("Resetting stale references to bundle #" + bundleID);
		j3.vm.OSGi.resetReferencesToBundle(bundleID);
	}
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void resetReferencesToBundle(String bundleName) throws Exception
	{
		resetReferencesToBundle(getBundle(bundleName));
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
