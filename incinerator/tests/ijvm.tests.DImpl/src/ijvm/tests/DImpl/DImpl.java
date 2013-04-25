package ijvm.tests.DImpl;

import java.util.ArrayList;

import ijvm.tests.C.C;
import ijvm.tests.D.D;

public class DImpl
	implements D
{
	public ArrayList<C> c;
	
	public DImpl()
	{
		c = new ArrayList<C>();
	}
	
	public void performD()
	{
		System.out.println("DImpl.performB");
	}
}
