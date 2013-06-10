package ijvm.tests.AlarmImpl;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Scanner;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.osgi.framework.BundleContext;
import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

class AlarmConfig_v2
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
		
		loadXMLConfig();
	}
	
	void loadXMLConfig() throws Exception
	{
		if (!configIsXML())
			migrateConfig();
			
		InputSource is = new InputSource(
			new InputStreamReader(
				new FileInputStream(config_file), "UTF-8"));
		is.setEncoding("UTF-8");
		
		DefaultHandler xmlHandler = new DefaultHandler() {
			boolean config = false, temperature = false, gaz = false, min = false, max = false;
			
			public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
				if (!config && qName.equalsIgnoreCase("config"))
					config = true;
				else if (config && !temperature && qName.equalsIgnoreCase("temperature"))
					temperature = true;
				else if (temperature && !min && qName.equalsIgnoreCase("min"))
					min = true;
				else if (temperature && !max && qName.equalsIgnoreCase("max"))
					max = true;
				else if (config && !gaz && qName.equalsIgnoreCase("gaz"))
					gaz = true;
				else if (gaz && !max && qName.equalsIgnoreCase("max"))
					max = true;
			}
			
		    public void endElement (String uri, String localName, String qName) throws SAXException {
				if (config && qName.equalsIgnoreCase("config"))
					config = false;
				else if (temperature && qName.equalsIgnoreCase("temperature"))
					temperature = false;
				else if (gaz && qName.equalsIgnoreCase("gaz"))
					gaz = false;
				else if (min && qName.equalsIgnoreCase("min"))
					min = false;
				else if (max && qName.equalsIgnoreCase("max"))
					max = false;
		    }

		    public void characters (char ch[], int start, int length) throws SAXException {
		    	if (temperature && min)
		    		min_temperature = Double.parseDouble(new String(ch, start, length));
		    	else if (temperature && max)
		    		max_temperature = Double.parseDouble(new String(ch, start, length));
		    	else if (gaz && max)
		    		max_gaz = Double.parseDouble(new String(ch, start, length));
		    }
		};
		
		SAXParser parser = SAXParserFactory.newInstance().newSAXParser();
		parser.parse(is, xmlHandler);
		
		System.out.println(getClass().getName() + ": XML config loaded");
	}

	boolean configIsXML() throws IOException
	{
		FileReader fr =  null;
		boolean r = false;
		
		try {
			fr = new FileReader(config_file);
			r = (fr.read() == '<');
		} finally {			
			fr.close();
		}
		return r;
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
		
		System.out.println(getClass().getName() + ": XML config initialized");
	}
	
	void saveConfig() throws Exception
	{
		FileWriter fw = new FileWriter(config_file);
		
		try {
			fw.write(
				"<config>\n" +
				" <temperature>\n" +
				"  <min>" + min_temperature + "</min>\n" +
				"  <max>" + max_temperature + "</max>\n" +
				" </temperature>\n" +
				" <gaz>\n" +
				"  <max>" + max_gaz + "</max>\n" +
				" </gaz>\n" +
				"</config>\n");
			
			fw.flush();
		} finally {
			fw.close();
		}
		
		System.out.println(getClass().getName() + ": XML config saved");
	}
	
	void migrateConfig() throws Exception
	{
		System.out.println(getClass().getName() + ": Migrating config from simple to XML");
		
		loadSimpleConfig();
		saveConfig();
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
