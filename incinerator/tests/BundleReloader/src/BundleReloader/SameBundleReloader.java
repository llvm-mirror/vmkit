package BundleReloader;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

public class SameBundleReloader
	implements Runnable
{
	final BundleContext context;
	final Thread worker;
	final boolean reinstall_is_update;
	final String bundle_path;
	
	BundleReloader reloader;
	
	volatile boolean cancelWork;
	volatile long loopCount;
	
	public SameBundleReloader(BundleContext context, boolean reinstall_is_update, boolean stale_ref_elimination, String bundle_path)
	{
		this.context = context;
		this.reinstall_is_update = reinstall_is_update;
		this.bundle_path = bundle_path;
		
		reloader = new BundleReloader(context, reinstall_is_update, stale_ref_elimination);
		
		cancelWork = true;
		worker = new Thread(this, "SameBundleReloader");
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
		System.out.println(reinstall_is_update ?
			"Bundle update running..." :
			"Bundle reinstallation running...");
		
		try {Thread.sleep(5000);}
		catch (Exception e) {}
		
		return !cancelWork;
	}
	
	void runTask()
	{
		if (reinstall_is_update)
			runTaskUpdate();
		else
			runTaskReinstall();
	}
	
	void runTerm()
	{
		cancelWork = true;
		System.out.println("Bundle reinstallation done. Count: " + loopCount);
	}
	
	void runTaskUpdate()
	{
		Bundle[] bundles = new Bundle[1];
		try {
			bundles[0] = reloader.startBundleUpdating(bundle_path);

			for (loopCount = 0; !cancelWork; ++loopCount)
				reloader.updateBundle(bundles);
		} catch (Throwable e) {
			e.printStackTrace();
		}
		
		if (bundles[0] != null) {
			//try {bundles[0].uninstall();}
			//catch (Throwable e) {e.printStackTrace();}
		}
	}
	
	void runTaskReinstall()
	{
		try {
			for (loopCount = 0; !cancelWork; ++loopCount)
				reloader.reinstallBundle(bundle_path);
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
}
