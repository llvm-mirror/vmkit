package BundleReloader;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.Bundle;
import org.osgi.framework.ServiceReference;
import org.osgi.service.packageadmin.PackageAdmin;

public class Activator
	implements BundleActivator, Runnable
{
	static final String targetBundle = "file:///home/koutheir/PhD/VMKit/knopflerfish/osgi/jars/http/http_all-3.1.2.jar";

	BundleContext context;
	ServiceReference pkgAdminRef;
	PackageAdmin pkgAdmin;
	boolean onIncinerator;
	
	Thread worker;
	volatile boolean cancelWork;
	volatile long loopCount;

	public void start(BundleContext context) throws Exception
	{
		this.context = context;
		cancelWork = false;
						
		onIncinerator = isRunningOnIncinerator();
		
		pkgAdminRef = context.getServiceReference(
					"org.osgi.service.packageadmin.PackageAdmin");
		pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
			
		worker = new Thread(this, "BundleReloader");
		worker.start();
	}
	
	public void stop(BundleContext context) throws Exception
	{
		if (!cancelWork) {
			cancelWork = true;
			worker.join();
		}
		worker = null;
		
		context.ungetService(pkgAdminRef);
		
		this.context = null;
		System.out.println("Bundle reinstallation count: " + loopCount);
	}

	public void run()
	{
		System.out.println("Bundle reinstallation running...");
		System.out.println("Running on Incinerator: " + (onIncinerator ? "yes" : "no"));
		
		try {Thread.sleep(2000);}
		catch (Exception e) {}
		
		try {
			for (loopCount = 0; !cancelWork; ++loopCount)
				reinstallBundle(targetBundle);
		} catch (Throwable e) {e.printStackTrace();}
		
		cancelWork = true;
		
		System.out.println("Bundle reinstallation done.");
		
		try {stopThisBundle();}
		catch (Throwable e) {e.printStackTrace();}
	}
	
	void reinstallBundle(String bundle_location) throws Throwable
	{
		Bundle[] bundles = new Bundle[] {
			context.installBundle(bundle_location)
		};
		
		long bundleID = bundles[0].getBundleId();
		try {
			bundles[0].start();
			Thread.sleep(100);
			
			if (onIncinerator)
				j3.vm.OSGi.setBundleStaleReferenceCorrected(bundleID, true);
	
			bundles[0].stop();
			bundles[0].uninstall();
		
			pkgAdmin.refreshPackages(bundles);
			System.gc();
			
			if (onIncinerator)
				j3.vm.OSGi.notifyBundleUninstalled(bundleID);
		} catch (Throwable e) {
			try {
				bundles[0].uninstall();
				pkgAdmin.refreshPackages(bundles);
			} catch (Exception ignore) {}
			
			throw e;
		}
	}
	
	void stopThisBundle() throws Exception
	{
		Bundle thisBundle = context.getBundle();
		int currentState = thisBundle.getState();
		
		if (currentState != Bundle.ACTIVE && currentState != Bundle.STARTING)
			return;
		
		thisBundle.stop();
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
