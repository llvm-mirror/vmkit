package ijvm.tests.AImpl;

import ijvm.tests.C.C;

public class InfiniteResurrecter
{
	C c;
	Object target;
	
	public InfiniteResurrecter(C c, Object o)
	{
		System.out.println(getClass().getName() + ": resurrecting " + o);
		
		this.c = c;
		this.target = o;
		
		c.registerObject(this);
	}
	
	protected void finalize()
	{
		new InfiniteResurrecter(this.c, target);
	}
}
