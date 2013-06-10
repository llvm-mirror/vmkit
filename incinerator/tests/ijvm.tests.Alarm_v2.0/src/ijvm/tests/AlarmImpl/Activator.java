package ijvm.tests.AlarmImpl;

import ijvm.tests.Alarm.Alarm;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceRegistration;

public class Activator
	implements BundleActivator
{
	BundleContext context;
	AlarmImpl_v2 alarm;
	ServiceRegistration alarmServiceReg;

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println(getClass().getName() + " provides " + Alarm.class.getName());
		context = bundleContext;
		
		alarm = new AlarmImpl_v2();
		alarm.open(context);
		
		alarmServiceReg = context.registerService(Alarm.class.getName(), alarm, null);
	}
	
	public void stop(BundleContext bundleContext) throws Exception
	{
		alarmServiceReg.unregister();
		alarm.close();
		
		System.out.println(getClass().getName() + " no more provides " + Alarm.class.getName());
	}
}
