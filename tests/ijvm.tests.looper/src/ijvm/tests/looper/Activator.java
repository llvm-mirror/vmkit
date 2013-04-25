package ijvm.tests.looper;

import ijvm.tests.logging.InlineFormatter;

import java.util.Hashtable;
import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;



public class Activator
	implements BundleActivator
{
	private Logger log;
	private LoopController service;
	
	public void start(BundleContext context) throws Exception
	{
		log = InlineFormatter.createLogger(Activator.class.getName());
		log.info("STARTING");

		service = new LoopControllerImpl(log);
		
		context.registerService(LoopController.class.getName(), service, new Hashtable());
	}
	
	public void stop(BundleContext context) throws Exception
	{
		log.info("Stopping...");

		service.cancelLoop();
		service = null;
		
		log.info("DONE");
		log = null;
	}
}
