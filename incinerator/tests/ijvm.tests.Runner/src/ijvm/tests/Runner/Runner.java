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
		System.out.println("Cancelling runner thread: " + Thread.currentThread().getName());
		cancelRunning = true;
	}
	
	public void run()
	{
		System.out.println("Started runner thread: " + Thread.currentThread().getName());
		
		try {
			while (!cancelRunning) {
				long delay = 2000 + (long)(Math.random() * 1000.0);
				
				synchronized(obj) {
					while (sleeping)
						obj.wait();
					sleeping = true;
					
					System.out.println(Thread.currentThread().getName() + ": sleeping for " + delay);
					Thread.sleep(delay);
					System.out.println(Thread.currentThread().getName() + ": woke up");
					
					sleeping = false;
					obj.notifyAll();
				}
				
				Thread.sleep(200);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		System.out.println("Stopped runner thread: " + Thread.currentThread().getName());
	}
}
