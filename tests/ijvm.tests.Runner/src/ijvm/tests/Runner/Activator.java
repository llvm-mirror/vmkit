package ijvm.tests.Runner;

import ijvm.tests.A.A;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.BundleException;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, Runnable
{
	private BundleContext context;
	private ServiceTracker aST;
	private A a;
	private Runner runner1, runner2;
	private Thread runnerThread1, runnerThread2, cancellerThread;

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("Started runner bundle.");
		context = bundleContext;
		
		aST = new ServiceTracker(context, A.class.getName(), null);
		aST.open();
		
		a = (A)aST.getService();
		if (a == null) {
			aST.close();
			aST = null;
			
			throw new BundleException("Runner bundle could not get A @ startup");
		}
		System.out.println("Runner bundle got A @ startup.");

		runnerThread1 = new Thread(runner1 = new Runner(a), "Runner 1");
		runnerThread2 = new Thread(runner2 = new Runner(a), "Runner 2");
		runnerThread1.start();
		runnerThread2.start();
		
		cancellerThread = new Thread(this, "Runner Canceller");
		cancellerThread.start();
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("Stopped runner bundle.");
		
		context = null;
		
		System.out.println("Runner bundle lost A but keeps a stale reference to it");
		aST.close();
		aST = null;
		// a = null;
		
//		runner.join();
	}

	public void run()
	{
		try {
			Thread.sleep(5000);
			runner1.cancel();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
