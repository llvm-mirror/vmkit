package j3mgr;

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
	long bundleToKill;

	public J3MgrImpl()
	{
		bundleToKill = -1;
	}
	
	public void open(BundleContext bundleContext)
	{
		context = bundleContext;
		
		// We need to know when packages are refreshed
		context.addFrameworkListener(this);
	}

	public void close()
	{
		context.removeFrameworkListener(this);
		context = null;
	}

	public void resetReferencesToBundle(Bundle bundle) throws Exception
	{
		System.out.println("resetReferencesToBundle: #" + bundle.getBundleId() + " " + bundle.getSymbolicName());
		
		bundleToKill = bundle.getBundleId();
		
		System.out.println("Stopping bundle: " + bundle.getSymbolicName());
		bundle.stop();
		System.out.println("Uninstalling bundle: " + bundle.getSymbolicName());
		bundle.uninstall();

		System.out.println("Refreshing framework...");
		refreshFramework(bundle);
	}
	
	void refreshFramework(Bundle bundle)
	{
		ServiceReference<?> pkgAdminRef = (ServiceReference<?>)context.getServiceReference(
			"org.osgi.service.packageadmin.PackageAdmin");
		PackageAdmin pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
		pkgAdmin.refreshPackages(new Bundle[] {bundle});		
	}

	public void frameworkEvent(FrameworkEvent event)
	{
		if (event.getType() != FrameworkEvent.PACKAGES_REFRESHED) return;

		if (bundleToKill == -1) return;
		
		System.out.println("Resetting stale references to bundle #" + bundleToKill);
		j3.vm.OSGi.resetReferencesToBundle(bundleToKill);
		
		bundleToKill = -1;
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
}
