package ijvm.tests.AImpl;

import java.util.ArrayList;

import ijvm.tests.A.A;
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
	private ServiceTracker cST;
	private ArrayList<C> c;
	private A a;

	public Activator()
	{
		c = new ArrayList<C>();
	}

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("AImpl provides A");
		context = bundleContext;

		cST = new ServiceTracker(context, C.class.getName(), null);
		cST.open();
		
		C service = (C)cST.getService();
		if (service != null)
			c.add(service);
		
		context.addServiceListener(this, "(objectclass=" + C.class.getName() + ")");

		if (c != null)
			System.out.println("AImpl got C @ startup");
		
		a = new AImpl();
		context.registerService(A.class.getName(), a, null);	
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("AImpl no more provides A");
		
		context.removeServiceListener(this);
		context = null;

		System.out.println("AImpl lost C but keeps a stale reference to it");
		cST.close();
		cST = null;
		// c = null;
		
		a = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object service = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (C.class.isInstance(service)) {
				System.out.println("AImpl got C");
				c.add((C)service);
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (C.class.isInstance(service)) {
				System.out.println("AImpl lost C but keeps a stale reference to it");
			}
			break;
		}
	}
}
