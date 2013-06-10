package ijvm.tests.AlarmImpl;

import org.osgi.framework.BundleContext;

import ijvm.tests.Alarm.Alarm;

public class AlarmImpl_v1
	implements Alarm
{
	boolean state;
	AlarmConfig_v1 config;
	
	AlarmImpl_v1()
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
		config = new AlarmConfig_v1();
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
		return state;
	}

	public void setMinTemperatureThreshold(double value) throws Exception
	{
		config.setMinTemperatureThreshold(value);
	}

	public double minTemperatureThreshold()
	{
		return config.minTemperatureThreshold();
	}

	public void setMaxTemperatureThreshold(double value) throws Exception
	{
		config.setMaxTemperatureThreshold(value);
	}

	public double maxTemperatureThreshold()
	{
		return config.maxTemperatureThreshold();
	}

	public void setMaxGazThreshold(double value) throws Exception
	{
		config.setMaxGazThreshold(value);
	}

	public double maxGazThreshold()
	{
		return config.maxGazThreshold();
	}

	public void saveConfig() throws Exception
	{
		config.saveConfig();
	}
}
