package ijvm.tests.staticexposer;

import java.util.Hashtable;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class Activator
	implements BundleActivator
{
	private StaticExposer service;
	
	public void start(BundleContext context) throws Exception
	{
		service = new StaticExposerImpl();
		
		Hashtable properties = new Hashtable();
		context.registerService(StaticExposer.class.getName(), service, properties);
	}

	public void stop(BundleContext context) throws Exception
	{
		service = null;
	}
}
