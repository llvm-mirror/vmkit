package ijvm.tests.tierImpl;

import ijvm.tests.looper.CallBack;
import ijvm.tests.looper.LoopController;
import ijvm.tests.tier.TierService;
import ijvm.tests.tier.Utility;

import java.util.ArrayList;
import java.util.logging.Logger;

public class TierServiceImpl
	implements TierService, CallBack
{
	private Logger log;
	private LoopController loopControllerService;
	
	TierServiceImpl(Logger log)
	{
		this.loopControllerService = null;
		this.log = log;
	}
	
	public void setLoopControllerService(LoopController loopControllerService)
	{
		this.loopControllerService = loopControllerService;
	}

	public void doSomething() throws Exception
	{
		log.info("Now doing something");
		
		try {
			thenAnotherThing();
		} catch (Exception e) {
			log.info(TierServiceImpl.class.getName() + ".doSomething");
			e.printStackTrace();
		}
		
		log.info("Done something");
	}
	
	public void thenAnotherThing() throws Exception
	{
		try {
			beforeLeaving();
		} catch (Exception e) {
			log.info(TierServiceImpl.class.getName() + ".thenAnotherThing");
			e.printStackTrace();
		}
	}
	
	public void beforeLeaving() throws Exception
	{
		try {
			loopControllerService.callMeBack(this);
		} catch (Exception e) {
			log.info(TierServiceImpl.class.getName() + ".beforeLeaving");
			e.printStackTrace();
		}
	}

	public void callBack() throws Exception
	{
		try {
			loopControllerService.keepThis(this);
			loopControllerService.loop();
		} catch (Exception e) {
			log.info(TierServiceImpl.class.getName() + ".callBack");
			e.printStackTrace();
		}
	}

	public Utility getSomething()
	{
		return new UtilityImpl(22);
	}
}
