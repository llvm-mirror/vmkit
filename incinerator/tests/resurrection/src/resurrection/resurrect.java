package resurrection;

public class resurrect
{
	public static volatile resurrected objRef = null;

	public static void main(String args[])
	{
		newUnreachable();
		for (int i=0; i < 2; ++i)
			makeUnreachable(i);
	}
	
	static void newUnreachable()
	{
		new resurrected();
	}
	
	static void makeUnreachable(int i)
	{
		System.out.println("Making objRef unreachable, round " + (i + 1)); 
		
		objRef = null;
		collectObject();
	}
	
	static void collectObject()
	{
		System.out.println("objRef = " + objRef); 
		
		try {
			System.out.println("Run GC..."); 
			System.gc();
			Thread.sleep(2000);
		} catch (Exception ignored) {}
		
		System.out.println("After GC..."); 
		System.out.println("objRef = " + objRef);
	}
}
