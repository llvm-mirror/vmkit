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
	BundleContext context;
	Thread worker;
	boolean cancelWork;
	long firstBundleID;
	ServiceTracker j3mgrST;
	J3Mgr j3mgr;
	boolean correctStaleReferences;

	public void start(BundleContext bundleContext) throws Exception
	{
		context = bundleContext;

		j3mgrST = new ServiceTracker(context, J3Mgr.class.getName(), null);
		j3mgrST.open();
		j3mgr = (J3Mgr)j3mgrST.getService();
		if (j3mgr == null)
			throw new BundleException("J3 Management service must be started before this service.");
		
		correctStaleReferences = true;
		firstBundleID = 13;
		
		cancelWork = false;
		worker = new Thread(this, "Stresser");
		worker.start();
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		cancelWork = true;
		
		if (worker != null) {
			worker.join();
			worker = null;
		}
		
		context = null;
	}

	public void run()
	{
		System.out.println("Bundle management stress running...");
		
		try {
			uninstallBundle(context.getBundle(firstBundleID));
						
			while (!cancelWork) {
				Bundle bundle = context.installBundle("file:///home/koutheir/PhD/VMKit/vmkit/incinerator/tests/plugins/ijvm.tests.AImpl_1.0.0.jar");
				bundle.start();
				Thread.sleep(20);
				
				System.out.println("Bundle started: " + bundle.getBundleId());
				
				uninstallBundle(bundle);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		System.out.println("Bundle management stress done.");
	}
	
	void uninstallBundle(Bundle bundle)
	{
		try {
			if (correctStaleReferences) {
				j3mgr.resetReferencesToBundle(bundle);
			} else {
				bundle.stop();
				bundle.uninstall();
			}
			
			System.gc();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
