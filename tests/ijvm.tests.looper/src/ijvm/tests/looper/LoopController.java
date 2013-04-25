package ijvm.tests.looper;

public interface LoopController
{
	public void loop() throws Exception;
	public void callMeBack(CallBack cb) throws Exception;
	public void cancelLoop();
	public void keepThis(Object o);
}
