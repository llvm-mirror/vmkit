package ijvm.tests.tierImpl;

import ijvm.tests.logging.InlineFormatter;
import ijvm.tests.looper.LoopController;
import ijvm.tests.tier.TierService;

import java.util.Hashtable;
import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;


public class Activator
	implements BundleActivator, ServiceListener, Runnable
{
	private BundleContext context;
	private Logger log;
	
	private ServiceTracker loopControllerTracker;
	private LoopController loopControllerService;
	
	private TierServiceImpl serviceImpl;
	
	private Thread thread;
	private boolean stopThread;

	
	public void start(BundleContext context) throws Exception
	{
		this.context = context;
		log = InlineFormatter.createLogger(Activator.class.getName());
		log.info("STARTING");

		loopControllerTracker = new ServiceTracker(context, LoopController.class.getName(), null);
		loopControllerTracker.open();
		loopControllerService = (LoopController)loopControllerTracker.getService();
		context.addServiceListener(this, "(objectclass=" + LoopController.class.getName() + ")");

		serviceImpl = new TierServiceImpl(log);
		serviceImpl.setLoopControllerService(loopControllerService);
		
		context.registerService(TierService.class.getName(), serviceImpl, new Hashtable());

//		thread = new Thread(this, "Tier worker");
//		stopThread = false;
//		thread.start();
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		log.info("Stopping...");

		this.context = null;
		
		if (thread != null) {
			stopThread = true;
			thread.join();
			thread = null;
		}
		
		serviceImpl = null;

		loopControllerTracker.close();
		loopControllerTracker = null;
		loopControllerService = null;

		log.info("DONE");
		log = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object serviceObj = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (LoopController.class.isInstance(serviceObj))
				serviceImpl.setLoopControllerService((LoopController)serviceObj);
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (LoopController.class.isInstance(serviceObj))
				serviceImpl.setLoopControllerService(null);
			break;
		}
	}

	public void run()
	{
		log.info("Tier thread running...");
		
//		for (;;) {}
		
		try {
/*
			boolean done;
			for (done = false; !done; ) {
				done = false;
			}
			
			Thread.sleep(1000);
*/
			while (!stopThread) {
				Thread.sleep(1000);
			}
		} catch (InterruptedException e) {
			log.info(Activator.class.getName() + ".run");
			e.printStackTrace();
		}

		log.info("Tier thread done.");
	}
}
