package ijvm.tests.CImpl;

import java.util.ArrayList;

import ijvm.tests.B.B;
import ijvm.tests.C.C;

public class CImpl
	implements C
{
	ArrayList<Object> registeredObjects;
	public ArrayList<B> b;
	
	CImpl()
	{
		b = new ArrayList<B>();
		registeredObjects = new ArrayList<Object>();
	}
	
	public void performC()
	{
		System.out.println("CImpl.performC");
	}

	public void registerObject(Object o)
	{
		registeredObjects.add(o);
	}
}
