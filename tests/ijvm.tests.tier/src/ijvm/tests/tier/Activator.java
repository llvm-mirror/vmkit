package ijvm.tests.tier;

import ijvm.tests.logging.InlineFormatter;

import java.util.logging.Logger;

import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class Activator
	implements BundleActivator
{
	private Logger log;

	public void start(BundleContext bundleContext) throws Exception
	{
		log = InlineFormatter.createLogger(Activator.class.getName());
		log.info("STARTING");
	}

	public void stop(BundleContext bundleContext) throws Exception
	{
		log.info("DONE");
		log = null;
	}
}
