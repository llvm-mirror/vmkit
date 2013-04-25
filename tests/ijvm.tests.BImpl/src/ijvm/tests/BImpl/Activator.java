package ijvm.tests.BImpl;

import java.util.ArrayList;

import ijvm.tests.A.A;
import ijvm.tests.B.B;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, ServiceListener
{
	private BundleContext context;

	private ServiceTracker aST;
	private ArrayList<A> a;
	private BImpl b;
	
	public Activator()
	{
		a = new ArrayList<A>();
	}

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("BImpl exports and provides B");
		context = bundleContext;

		aST = new ServiceTracker(context, A.class.getName(), null);
		aST.open();
		
		A service = (A)aST.getService();
		if (service != null)
			a.add(service);
		
		context.addServiceListener(this, "(objectclass=" + A.class.getName() + ")");

		if (a != null)
			System.out.println("BImpl got A @ startup");
		
		b = new BImpl();
		context.registerService(B.class.getName(), b, null);
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		System.out.println("BImpl no more provides B");

		context.removeServiceListener(this);
		context = null;
		
		System.out.println("BImpl lost A but keeps a stale reference to it");
		aST.close();
		aST = null;
		// a = null;
		
		b = null;
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object service = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (A.class.isInstance(service)) {
				System.out.println("BImpl got A");
				a.add((A)service);
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (A.class.isInstance(service)) {
				System.out.println("BImpl lost A but keeps a stale reference to it");
			}
			break;
		}
	}
}
