package BundleReloader;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

public class ScanningBundleReloader
	implements Runnable
{
	final BundleContext context;
	final Thread worker;
	
	BundleReloader reloader;
	
	volatile boolean cancelWork;
	volatile long currentBundleID;
	
	public ScanningBundleReloader(BundleContext context)
	{
		this.context = context;
		currentBundleID = context.getBundle().getBundleId();
		
		reloader = new BundleReloader(context, true, true);
		
		cancelWork = true;
		worker = new Thread(this, "ScanningBundleReloader");
	}
	
	public void start()
	{
		cancelWork = false;
		worker.start();
	}
	
	public void stop() throws Exception
	{
		if (cancelWork) return;

		cancelWork = true;
		worker.join();
	}
	
	public void run()
	{
		if (runInit())
			runTask();
		runTerm();
	}	
	
	boolean runInit()
	{
		System.out.println("Bundle update running...");
		
		try {Thread.sleep(1000);}
		catch (Exception e) {}
		
		return !cancelWork;
	}
	
	void runTask()
	{
		Bundle[] bundles = new Bundle[1];
		try {
			for (bundles[0] = getNextBundle();
				(bundles[0] != null) && (!cancelWork);
				bundles[0] = getNextBundle())
			{
				System.out.println("Bundle update: #" + currentBundleID + " (" + bundles[0].getSymbolicName() + ")");
				reloader.updateBundle(bundles);
				Thread.sleep(4000);
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	
	void runTerm()
	{
		cancelWork = true;
		System.out.println("Bundle reinstallation done.");
	}
	
	Bundle getNextBundle()
	{
		return context.getBundle(++currentBundleID);
	}
}
