package ijvm.tests.AlarmImpl;

import org.osgi.framework.BundleContext;

import ijvm.tests.Alarm.Alarm;

public class AlarmImpl_v2
	implements Alarm
{
	boolean state;
	AlarmConfig_v2 config;
	
	AlarmImpl_v2()
	{
		System.out.println(getClass().getName() + " loaded");
		
		state = false;
	}

	protected void finalize()
	{
		System.out.println(getClass().getName() + " unloaded");
	}
	
	public void open(BundleContext bundleContext) throws Exception
	{
		config = new AlarmConfig_v2();
		config.open(bundleContext);
	}
	
	public void close() throws Exception
	{
		config.close();
	}
	
	public void setState(boolean on)
	{
		state = on;
	}

	public boolean state()
	{
		setState(Math.random() >= 0.5);
		return state;
	}

	public void setMinTemperatureThreshold(double value) throws Exception
	{
	}

	public double minTemperatureThreshold()
	{
		return 0.0;
	}

	public void setMaxTemperatureThreshold(double value) throws Exception
	{
	}

	public double maxTemperatureThreshold()
	{
		return 0.0;
	}

	public void setMaxGazThreshold(double value) throws Exception
	{
	}

	public double maxGazThreshold()
	{
		return 0.0;
	}

	public void saveConfig() throws Exception
	{
		config.saveConfig();
	}
}
