package BundleReloader;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceReference;
import org.osgi.service.packageadmin.PackageAdmin;

public class BundleReloader
{
	final BundleContext context;
	final boolean on_incinerator, stale_ref_elimination, reinstall_is_update;
	
	ServiceReference pkg_admin_ref;
	PackageAdmin pkg_admin;

	public BundleReloader(BundleContext context, boolean reinstall_is_update, boolean stale_ref_elimination)
	{
		this.context = context;
		this.reinstall_is_update = reinstall_is_update;
		this.stale_ref_elimination = stale_ref_elimination;
		
		pkg_admin_ref = context.getServiceReference(
			"org.osgi.service.packageadmin.PackageAdmin");
		pkg_admin = (PackageAdmin)context.getService(pkg_admin_ref);
		
		on_incinerator = isRunningOnIncinerator();
		System.out.println("Running on Incinerator: " + (on_incinerator ? "yes" : "no"));
	}
	
	protected void finalize()
	{
		context.ungetService(pkg_admin_ref);
	}

	public void reinstallBundle(String bundle_location) throws Throwable
	{
		Bundle[] bundles = new Bundle[] {
			context.installBundle(bundle_location)
		};
		
		long bundleID = bundles[0].getBundleId();
		try {
			bundles[0].start();
			Thread.sleep(100);
			
			if (on_incinerator)
				j3.vm.OSGi.setBundleStaleReferenceCorrected(bundleID, stale_ref_elimination);
	
			bundles[0].stop();
			bundles[0].uninstall();
		
			pkg_admin.refreshPackages(bundles);
			System.gc();
			
			if (on_incinerator)
				j3.vm.OSGi.notifyBundleUninstalled(bundleID);
		} catch (Throwable e) {
			try {
				bundles[0].uninstall();
				pkg_admin.refreshPackages(bundles);
			} catch (Exception ignore) {}
			
			throw e;
		}
	}
	
	public Bundle startBundleUpdating(String bundle_location) throws Exception
	{
		Bundle bundle = context.installBundle(bundle_location);
		bundle.start();
		Thread.sleep(300);
		
		if (on_incinerator)
			j3.vm.OSGi.setBundleStaleReferenceCorrected(bundle.getBundleId(), stale_ref_elimination);
		
		return bundle;
	}
	
	public void updateBundle(Bundle[] bundles) throws Throwable
	{
		Throwable updateException = null;
		for (int retry = 10; retry > 0; --retry) {
			try {
				bundles[0].update();
				
				updateException = null;
				retry = 0;
			} catch (IllegalStateException e) {
				updateException = e;
				Thread.sleep(500);
			}
		}
		
		if (updateException != null)
			throw updateException;
		
		pkg_admin.refreshPackages(bundles);
		Thread.sleep(200);
		System.gc();
	}
	
	public void installBundle(String bundle_location) throws Exception
	{
		context.installBundle(bundle_location);
	}
	
	boolean isRunningOnIncinerator()
	{
		boolean r = false;
		try {
			j3.vm.OSGi.isBundleStaleReferenceCorrected(
				context.getBundle().getBundleId());
			r = true;
		}
		catch (UnsatisfiedLinkError ignore) {}
		catch (Throwable ignore) {r = true;}
		return r;
	}
}
