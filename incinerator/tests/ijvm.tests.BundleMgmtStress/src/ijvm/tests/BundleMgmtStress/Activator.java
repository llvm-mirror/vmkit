package ijvm.tests.BundleMgmtStress;

import j3.J3Mgr;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.BundleException;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, Runnable
{
	static final boolean correctStaleReferences = false;
	static final String targetBundle = "file:///home/koutheir/PhD/VMKit/knopflerfish.verbatim/osgi/jars/http/http_all-3.1.2.jar";
	static final long firstBundleID = 6;
	
	BundleContext context;
	Thread worker;
	volatile boolean cancelWork;
	ServiceTracker j3mgrST;
	J3Mgr j3mgr;
	long loopCount;

	public void start(BundleContext bundleContext) throws Exception
	{
		context = bundleContext;

		j3mgrST = new ServiceTracker(context, J3Mgr.class.getName(), null);
		j3mgrST.open();
		j3mgr = (J3Mgr)j3mgrST.getService();
		if (j3mgr == null) {
			throw new BundleException(
				"J3 Management service must be started before this service.");
		}
		
		loopCount = 0;
		
		cancelWork = false;
		worker = new Thread(this, "Stresser");
		worker.start();
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		if (!cancelWork) {
			cancelWork = true;
			
			if (worker != null) {
				worker.join();
				worker = null;
			}
		}
		
		System.out.println("Bundle reinstallation count: " + loopCount);
		context = null;
	}

	public void run()
	{
		System.out.println("Bundle management stress running...");
		try {
			Thread.sleep(2000);
		} catch (Exception e) {}
		
		try {
			uninstallBundle(context.getBundle(firstBundleID));

			while (!cancelWork) {
				Bundle bundle = context.installBundle(targetBundle);
				bundle.start();
				Thread.sleep(100);
								
				uninstallBundle(bundle);
			}
		} catch (Throwable e) {
			cancelWork = true;
			e.printStackTrace();
		}
		
		System.out.println("Bundle management stress done.");
		
		try {
			cancelWork = true;
			Bundle thisBundle = context.getBundle();
			int currentState = thisBundle.getState();
			
			if (currentState == Bundle.ACTIVE ||
				currentState == Bundle.STARTING) {
				thisBundle.stop();
			}
		} catch (BundleException e) {
			e.printStackTrace();
		}
	}
	
	void uninstallBundle(Bundle bundle) throws Throwable
	{	
		try {
			j3mgr.setBundleStaleReferenceCorrected(
				bundle.getBundleId(), correctStaleReferences);
		} catch (UnsatisfiedLinkError e) {
			if (correctStaleReferences)
				throw e;
		}
		
		bundle.stop();
		bundle.uninstall();
		System.out.println("Uninstalled: bundleID=" + bundle.getBundleId());
		
		loopCount++;
		
		System.gc();
	}
}
