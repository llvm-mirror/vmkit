package ijvm.tests.AlarmController;

import ijvm.tests.Alarm.Alarm;

class AlarmWatchDog
	implements Runnable
{
	volatile boolean cancel;
	volatile Thread thread;
	final Alarm alarm;
	
	public AlarmWatchDog(Alarm alarm)
	{
		this.alarm = alarm;
	}
	
	public Alarm alarm()
	{
		return alarm;
	}
	
	public void start() throws Exception
	{
		while (thread != null && thread.isAlive())
			stop();
		
		cancel = false;
		thread = new Thread(this, getClass().getName());
		thread.start();
	}
	
	public void stop() throws Exception
	{
		if (thread == null || !thread.isAlive()) return;
		
		cancel = true;
		thread.join();
	}
	
	public void run()
	{
		System.out.println(getClass().getName() + ": monitoring...");
		
		while (!cancel) {
			try {Thread.sleep(1000);} catch (Exception ignored) {}
			if (alarm.state() == false) continue;
			
			System.out.println("ALARM! ALARM! ALARM! ALARM!");
		}

		System.out.println(getClass().getName() + ": stopped.");
	}
}
