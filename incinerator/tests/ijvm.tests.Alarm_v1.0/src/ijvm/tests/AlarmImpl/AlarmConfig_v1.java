package ijvm.tests.AlarmImpl;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.Scanner;

import org.osgi.framework.BundleContext;

class AlarmConfig_v1
{
	File config_file;
	double min_temperature;
	double max_temperature;
	double max_gaz;
	
	public void open(BundleContext bundleContext) throws Exception
	{
		config_file = bundleContext.getDataFile("alarm.conf");
		loadConfig();
	}
	
	public void close() throws Exception
	{
		saveConfig();
	}
	
	void loadConfig() throws Exception
	{
		initConfig();
		
		if (!config_file.exists()) {
			saveConfig();
			return;
		}
		
		loadSimpleConfig();
	}
	
	void loadSimpleConfig() throws Exception
	{
		FileReader fr = new FileReader(config_file);
		Scanner s = new Scanner(fr);
		
		try {
			while (s.hasNextLine()) {
				if (s.hasNext("min_temperature")) {
					s.next("min_temperature");
					min_temperature = s.nextDouble();
				} else if (s.hasNext("max_temperature")) {
					s.next("max_temperature");
					max_temperature = s.nextDouble();
				} else if (s.hasNext("max_gaz")) {
					s.next("max_gaz");
					max_gaz = s.nextDouble();
				} else
					s.nextLine();
			}
		} finally {
			s.close();
		}
		
		System.out.println(getClass().getName() + ": Simple config loaded");
	}
	
	void initConfig()
	{
		min_temperature = -40.0;
		max_temperature = 60.0;
		max_gaz = 10.0;
		
		System.out.println(getClass().getName() + ": Simple config initialized");
	}
	
	void saveConfig() throws Exception
	{
		FileWriter fw = new FileWriter(config_file);
		
		try {
			fw.write(
				"min_temperature " + min_temperature + "\n" +
				"max_temperature " + max_temperature + "\n" +
				"max_gaz " + max_gaz + "\n");
			
			fw.flush();
		} finally {
			fw.close();
		}
		
		System.out.println(getClass().getName() + ": Simple config saved");
	}

	public void setMinTemperatureThreshold(double value) throws Exception
	{
		min_temperature = value;
		saveConfig();
	}

	public double minTemperatureThreshold()
	{
		return min_temperature;
	}

	public void setMaxTemperatureThreshold(double value) throws Exception
	{
		max_temperature = value;
		saveConfig();
	}

	public double maxTemperatureThreshold()
	{
		return max_temperature;
	}

	public void setMaxGazThreshold(double value) throws Exception
	{
		max_gaz = value;
		saveConfig();
	}

	public double maxGazThreshold()
	{
		return max_gaz;
	}

}
