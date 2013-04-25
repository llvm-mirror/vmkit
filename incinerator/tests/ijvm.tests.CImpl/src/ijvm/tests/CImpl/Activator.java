package ijvm.tests.CImpl;

import ijvm.tests.B.B;
import ijvm.tests.C.C;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, ServiceListener
{
	private BundleContext context;

	private ServiceTracker bST;
	private CImpl c;

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("CImpl exports and provides C");
		context = bundleContext;

		c = new CImpl();

		bST = new ServiceTracker(context, B.class.getName(), null);
		bST.open();
		
		B service = (B)bST.getService();
		if (service != null) {
			System.out.println("CImpl got B @ startup");
			
			c.b.add(service);
		}
		
		context.addServiceListener(this, "(objectclass=" + B.class.getName() + ")");
		context.registerService(C.class.getName(), c, null);
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("CImpl no more provides C");

		context.removeServiceListener(this);
		context = null;
		
		System.out.println("CImpl lost B but keeps a stale reference to it");
		bST.close();
		bST = null;
		// b = null;
		
		c = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object service = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (B.class.isInstance(service)) {
				System.out.println("CImpl got B");
				c.b.add((B)service);
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (B.class.isInstance(service)) {
				System.out.println("CImpl lost B but keeps a stale reference to it");
			}
			break;
		}
	}
}
