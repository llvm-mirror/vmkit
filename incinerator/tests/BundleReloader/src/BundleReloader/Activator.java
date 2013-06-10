package BundleReloader;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class Activator
	implements BundleActivator
{
	static final String target_bundle = "file:///home/koutheir/PhD/VMKit/knopflerfish/osgi/jars/http/http_all-3.1.2.jar";
	static final boolean reinstall_is_update = true;
	static final boolean stale_ref_elimination = false;

	BundleContext context;
	SameBundleReloader reloader;
	//ScanningBundleReloader reloader;
	
	public void start(BundleContext context) throws Exception
	{
		this.context = context;
		
		reloader = new SameBundleReloader(context, reinstall_is_update, stale_ref_elimination, target_bundle);
		//reloader = new ScanningBundleReloader(context);
		reloader.start();
	}
	
	public void stop(BundleContext context) throws Exception
	{
		reloader.stop();
		reloader = null;
		
		this.context = null;
	}
}
