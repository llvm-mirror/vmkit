package ijvm.tests.loopclient;

import java.util.ArrayList;
import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;

import ijvm.tests.logging.InlineFormatter;
import ijvm.tests.tier.TierService;
import ijvm.tests.tier.Utility;

public class Activator
	implements BundleActivator, ServiceListener, Runnable
{
	private BundleContext context;
	private Logger log;
	
	private ServiceTracker serviceTracker;
	private TierService service;
	
	private Thread thread;
	private ArrayList utilArray;

	public void start(BundleContext context) throws Exception
	{
		this.context = context;
		log = InlineFormatter.createLogger(Activator.class.getName());
		log.info("STARTING");
		
		serviceTracker = new ServiceTracker(context, TierService.class.getName(), null);
		serviceTracker.open();
		service = (TierService)serviceTracker.getService();
		context.addServiceListener(this, "(objectclass=" + TierService.class.getName() + ")");
		
		thread = new Thread(this, "Loop Client");
		thread.start();
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		log.info("Stopping...");

		this.context = null;
		
		thread.join();
		thread = null;

		serviceTracker.close();
		serviceTracker = null;
		service = null;

		log.info("DONE");
		log = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object serviceObj = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (TierService.class.isInstance(serviceObj))
				service = (TierService)serviceObj;
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (TierService.class.isInstance(serviceObj))
				service = null;
			break;
		}
	}

	public void run()
	{
		log.info("Loop client running...");
		
		utilArray = new ArrayList(1);
		utilArray.add(
			service.getSomething());
		
		try {			
			service.doSomething();
		} catch (Exception e) {
			log.info(Activator.class.getName() + ".run");
			e.printStackTrace();
		}

		try {
			((Utility)utilArray.get(0)).getSomething();
		} catch (Exception e) {
			log.info(Activator.class.getName() + ".run");
			e.printStackTrace();
		}

		log.info("Loop client done.");
	}
}
