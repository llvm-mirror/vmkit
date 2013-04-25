package ijvm.tests.looper;

import java.util.logging.Logger;

public class LoopControllerImpl
	implements LoopController
{
	protected boolean cancelLoop;
	private Logger log;
	private Object keptObj;
	
	public LoopControllerImpl(Logger log)
	{
		this.log = log;
	}

	public void loop() throws Exception
	{
		log.info("Start looping");
		
		cancelLoop = false;
		while (!cancelLoop) {
			log.info("Still looping...");
			Thread.sleep(10000);
		}
		
		log.info("Done looping");
	}

	public void cancelLoop()
	{
		log.info("Cancel looping");
		cancelLoop = true;
	}

	public void callMeBack(CallBack cb) throws Exception
	{
		try {
			cb.callBack();
		} catch (Exception e) {
			log.info(LoopControllerImpl.class.getName() + ".callMeBack");
			e.printStackTrace();
		}
	}

	public void keepThis(Object o)
	{
		keptObj = o;
	}
}
