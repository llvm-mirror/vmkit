package ijvm.tests.Alarm;

public interface Alarm
{
	public void setState(boolean on);
	public boolean state();
	
	public void setMinTemperatureThreshold(double value) throws Exception;
	public double minTemperatureThreshold();
	
	public void setMaxTemperatureThreshold(double value) throws Exception;
	public double maxTemperatureThreshold();
	
	public void setMaxGazThreshold(double value) throws Exception;
	public double maxGazThreshold();
	
	public void saveConfig() throws Exception;
}
