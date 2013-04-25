package ijvm.tests.Runner;

public class Runner
	implements Runnable
{
	private Object obj;
	private static boolean sleeping;
	private boolean cancelRunning;
	
	{
		sleeping = false;
	}
	
	public Runner(Object o)
	{
		cancelRunning = false;
		obj = o;
	}
	
	public void cancel()
	{
		cancelRunning = false;
	}
	
	public void run()
	{
		System.out.println("Started runner thread: " + Thread.currentThread().getName());
		
		try {
			while (!cancelRunning) {
				long delay = 2000 + (long)(Math.random() * 1000.0);
				
				ijvm_tests_Runner_loop(delay);
				
				Thread.sleep(200);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		System.out.println("Stopped runner thread: " + Thread.currentThread().getName());
	}
	
	void ijvm_tests_Runner_loop(long delay) throws Exception
	{
		synchronized(obj) {
			iteration(delay);
		}
	}
	
	void iteration(long delay) throws Exception
	{
		while (sleeping)
			obj.wait();
		sleeping = true;
		
		System.out.println(Thread.currentThread().getName() + ": sleeping for " + delay);
		Thread.sleep(delay);
		System.out.println(Thread.currentThread().getName() + ": woke up");
		
		sleeping = false;
		obj.notifyAll();
	}
}
