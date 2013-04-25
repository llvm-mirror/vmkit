package ijvm.tests.BImpl;

import ijvm.tests.A.A;
import ijvm.tests.A.Token;
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
	private BImpl b;
	
	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println("BImpl exports and provides B");
		context = bundleContext;

		b = new BImpl();
		
		aST = new ServiceTracker(context, A.class.getName(), null);
		aST.open();
		
		A service = (A)aST.getService();
		if (service != null) {
			System.out.println("BImpl got A @ startup");
			
			b.a.add(service);
			this.useA();
		}
		
		context.addServiceListener(this, "(objectclass=" + A.class.getName() + ")");
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
				b.a.add((A)service);
				
				this.useA();
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (A.class.isInstance(service)) {
				System.out.println("BImpl lost A but keeps a stale reference to it");
			}
			break;
		}
	}
	
	private void useA()
	{
		A oneA = b.a.get(b.a.size() - 1);
		Token token = oneA.getToken();
		token.getValue();
		
		b.tokens.add(token);
		
		System.out.println("BImpl got Token from A");
	}
}
