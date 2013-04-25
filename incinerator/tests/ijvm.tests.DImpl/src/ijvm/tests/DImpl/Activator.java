package ijvm.tests.DImpl;

import ijvm.tests.C.C;
import ijvm.tests.D.D;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, ServiceListener
{
	private BundleContext context;

	private ServiceTracker cST;
	private DImpl d;
	
	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("DImpl exports and provides D");
		context = bundleContext;

		d = new DImpl();

		cST = new ServiceTracker(context, C.class.getName(), null);
		cST.open();
		
		C service = (C)cST.getService();
		if (service != null) {
			System.out.println("DImpl got C @ startup");
			
			d.c.add(service);
		}
		
		context.addServiceListener(this, "(objectclass=" + C.class.getName() + ")");
		context.registerService(D.class.getName(), d, null);
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("DImpl no more provides D");

		context.removeServiceListener(this);
		context = null;
		
		System.out.println("DImpl lost C but keeps a stale reference to it");
		cST.close();
		cST = null;
		// c = null;
		
		d = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object service = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (C.class.isInstance(service)) {
				System.out.println("DImpl got C");
				d.c.add((C)service);
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (C.class.isInstance(service)) {
				System.out.println("DImpl lost C but keeps a stale reference to it");
			}
			break;
		}
	}
}
