package ijvm.tests.AlarmController;

import ijvm.tests.Alarm.Alarm;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.ServiceEvent;
import org.osgi.framework.ServiceListener;
import org.osgi.util.tracker.ServiceTracker;

public class Activator
	implements BundleActivator, ServiceListener
{
	BundleContext context;
	volatile Alarm alarm;
	ServiceTracker alarmServiceTracker;
	AlarmWatchDog watchdog;

	public void start(BundleContext bundleContext) throws Exception
	{
		System.out.println(getClass().getName() + " consumes " + Alarm.class.getName());
		context = bundleContext;
		
		alarmServiceTracker = new ServiceTracker(context, Alarm.class.getName(), null);
		alarmServiceTracker.open();
		
		alarm = (Alarm)alarmServiceTracker.getService();
		updateWatchDog();
		
		context.addServiceListener(this, "(objectclass=" + Alarm.class.getName() + ")");
	}
	
	public void stop(BundleContext bundleContext) throws Exception
	{
		if (alarm != null) {
			alarm.saveConfig();
			alarm = null;
		}
		updateWatchDog();
		
		context.removeServiceListener(this);
		alarmServiceTracker.close();

		System.out.println(getClass().getName() + " no more consumes " + Alarm.class.getName());
	}

	public void serviceChanged(ServiceEvent event)
	{
		Object service = context.getService(event.getServiceReference());
		
		switch(event.getType()) {
		case ServiceEvent.REGISTERED:
			if (Alarm.class.isInstance(service)) {
				if (alarm == null)
					alarm = (Alarm)service;
				updateWatchDog();
				System.out.println(getClass().getName() + " got a new " + Alarm.class.getName());
			}
			break;
			
		case ServiceEvent.UNREGISTERING:
			if (Alarm.class.isInstance(service)) {
				//alarm = null;
				updateWatchDog();
				System.out.println(getClass().getName() + " lost " + Alarm.class.getName());
			}
			break;
		}
	}
	
	void updateWatchDog()
	{
		try {
			if (alarm == null && watchdog != null) {
				// Stop the watch dog
				watchdog.stop();
				watchdog = null;
			} else if (alarm != null && watchdog == null) {
				// Start a new watch dog
				watchdog = new AlarmWatchDog(alarm);
				watchdog.start();
			} else if (alarm != null && watchdog != null && watchdog.alarm() != alarm) {
				// Modify the watch dog
				watchdog.stop();
				watchdog = new AlarmWatchDog(alarm);
				watchdog.start();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
