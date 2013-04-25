package ijvm.isolate;


import ijvm.isolate.isolatemanager.IsolateManager;
import ijvm.isolate.util.IsolateLogger;

import java.util.Hashtable;
import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class IsolateManagerActivator
	implements BundleActivator
{
	private Logger log;
	private IsolateManagerImpl serviceImpl;
	
	public void start(BundleContext context) throws Exception
	{
		log = IsolateLogger.createLogger(IsolateManagerActivator.class.getName());
		log.info("STARTING");
		
		// Create the service implementation
		serviceImpl = new IsolateManagerImpl(log);
		serviceImpl.open(context);
		
		// Register the service
		context.registerService(IsolateManager.class.getName(), serviceImpl, new Hashtable());
	}
	
	public void stop(BundleContext context) throws Exception
	{
		serviceImpl.close();
		serviceImpl = null;
		
		log.info("DONE");
		log = null;
	}
}
