package ijvm.tests.tierImpl;

import ijvm.tests.tier.Utility;

public class UtilityImpl
	implements Utility
{
	int something;
	
	UtilityImpl(int x)
	{
		something = x;
	}
	
	protected void finalize() throws Throwable
	{
		System.out.println("UtilityImpl.finalize()");
		something = -1;
	}
	
	public int getSomething()
	{
		return something;
	}
	
	public void setSomething(int x)
	{
		something = x;
	}
}
