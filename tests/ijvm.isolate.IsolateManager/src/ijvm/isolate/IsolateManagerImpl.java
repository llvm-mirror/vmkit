package ijvm.isolate;

import ijvm.isolate.isolatemanager.IsolateManager;
import ijvm.isolate.vm.IJVM;

import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.Dictionary;
import java.util.List;
import java.util.logging.Logger;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.FrameworkEvent;
import org.osgi.framework.FrameworkListener;
import org.osgi.framework.ServiceReference;
import org.osgi.framework.Version;
import org.osgi.service.packageadmin.PackageAdmin;


class IsolateManagerImpl
	implements IsolateManager, FrameworkListener
{
	private Logger log;
	private BundleContext context;
	private List isolatesToKill;

	public IsolateManagerImpl(Logger log)
	{
		this.log = log;
		isolatesToKill = new ArrayList();
	}

	public void open(BundleContext context)
	{
		this.context = context;
		// We need to know when packages are refreshed
		context.addFrameworkListener(this);
	}

	public void close()
	{
		context.removeFrameworkListener(this);
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
	
	public void killBundle(String bundleName) throws Exception
	{
		log.info("Killing bundle: " + bundleName);
		killBundles(new Bundle[] {getBundle(bundleName)});
		log.info("Killed bundle.");
	}
	
	public void killBundles(String bundleName1, String bundleName2) throws Exception
	{
		log.info("Killing bundles: " + bundleName1 + ", " + bundleName2);
		killBundles(new Bundle[] {getBundle(bundleName1), getBundle(bundleName2)});
		log.info("Killed bundles.");
	}

	public void killBundles(Bundle[] bundles) throws Exception
	{
		// We kill bundles asynchronously
		final Object intializedLock = new Object();
		final Bundle[] theBundles = bundles;
		
		Thread worker = new Thread(new Runnable() {
			public void run()
			{
				try {
					killBundlesThread(intializedLock, theBundles);
				} catch (Exception e) {
					log.info(IsolateManagerImpl.class.getName() + ".killBundles");
					e.printStackTrace();
				}
			}
		}, "Bundles killer");
		worker.start();
		
		synchronized (intializedLock) {
			intializedLock.wait();
		}
		// At this point, we are sure the bundle execution is denied
	}

	void killBundlesThread(final Object intializedLock, final Bundle[] bundles) throws Exception
	{
		synchronized (isolatesToKill) {
			while (!isolatesToKill.isEmpty()) {
				log.info("Previous killBundles operation pending...");
				isolatesToKill.wait(4000);
			}
		}

		long[] isolateID = new long[bundles.length];
		for (int i = 0; i < bundles.length; ++i) {
			String activatorClassName = (String)bundles[i].getHeaders().get("Bundle-Activator");
			Object activatorObject = bundles[i].loadClass(activatorClassName).newInstance();
			isolateID[i] = IJVM.getObjectIsolateID(activatorObject);
			activatorObject = null;
		}
		
		// Disable bundle execution
		log.info("Denying bundles execution...");
		IJVM.disableIsolates(isolateID, true, false);
		
		synchronized (intializedLock) {	// Enable caller to continue
			intializedLock.notify();
		}
		
		// Stop and uninstall the bundles
		// NOTE: As the bundle Activator.stop() method is specially patched to return directly
		// without doing anything, we can call it.
		log.info("Stopping and uninstalling bundles...");
		for (int i = 0; i < bundles.length; ++i) {
			bundles[i].stop();
			bundles[i].uninstall();
		}

		synchronized (isolatesToKill) {
			for (int i = 0; i < bundles.length; ++i)
				isolatesToKill.add(i, new Long(isolateID[i]));
			
			isolatesToKill.notifyAll();
		}
		
		log.info("Refreshing framework...");
		ServiceReference pkgAdminRef = context.getServiceReference("org.osgi.service.packageadmin.PackageAdmin");
		PackageAdmin pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
		pkgAdmin.refreshPackages(bundles);
	}
	
	public void frameworkEvent(FrameworkEvent event)
	{
		if (event.getType() != FrameworkEvent.PACKAGES_REFRESHED) return;

		log.info("Framework refreshed.");
		
		long[] isolateID = null;
		
		synchronized (isolatesToKill) {
			int isolateCount = isolatesToKill.size();
			if (isolateCount > 0) {
				isolateID = new long[isolateCount];
				for (int i = 0; i < isolateCount; ++i)
					isolateID[i] = ((Long)isolatesToKill.get(i)).longValue();
				
				isolatesToKill.clear();
				isolatesToKill.notifyAll();
			}
		}
		
		if (isolateID == null) return;

		log.info("Resetting isolate references...");
		IJVM.disableIsolates(isolateID, true, true);
	}
}
