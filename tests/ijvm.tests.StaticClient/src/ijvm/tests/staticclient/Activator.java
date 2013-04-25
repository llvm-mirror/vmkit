package ijvm.tests.staticclient;

import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.util.tracker.ServiceTracker;

import ijvm.tests.logging.InlineFormatter;
import ijvm.tests.staticexposer.StaticExposer;


public class Activator
	implements BundleActivator, Runnable
{
	private StaticExposer service;
	private ServiceTracker serviceTracker;
	private Thread thread;
	boolean stopThread;
	Logger log;
	
	public void start(BundleContext context) throws Exception
	{
		log = InlineFormatter.createLogger(Activator.class.getName());
		log.info("STARTING");
		
		serviceTracker = new ServiceTracker(context, StaticExposer.class.getName(), null);
		serviceTracker.open();
		service = (StaticExposer)serviceTracker.getService();

		stopThread = false;
		thread = new Thread(this, "Sync Client");
		thread.start();
	}
	
	public void stop(BundleContext context) throws Exception
	{
		stopThread = true;
		thread.join();
		thread = null;
		
		serviceTracker.close();
		serviceTracker = null;
		
		log.info("DONE");
		log = null;
	}
	
	public void run()
	{
		try {
			while (!stopThread) {
				log.info(">>|");
//				sync1();
				sync2();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private void sync1() throws InterruptedException
	{
//		synchronized(service.theStaticString) {
		synchronized(service.getStaticString()) {
			log.info("|*|");
			Thread.sleep(1000);
			log.info("|>>");
		}				
	}

	private void sync2() throws InterruptedException
	{
		synchronized(service.getStaticInteger()) {
			int r = xync3();
			int m = xync4();
			
			log.info("|*| r = " + r + ", m = " + m);
			Thread.sleep(1000);
			log.info("|>>");
		}				
	}
	
	private int xync3()
	{
		return service.getStaticInteger().intValue();
	}
	
	private int xync4()
	{
		return service.theStaticInteger.intValue();
	}
}
