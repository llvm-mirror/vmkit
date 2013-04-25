package ijvm.tests.AImpl;

import java.util.ArrayList;

import ijvm.tests.A.A;
import ijvm.tests.A.Token;
import ijvm.tests.C.C;

public class AImpl
	implements A
{
	public ArrayList<C> c;

	public AImpl()
	{
		c = new ArrayList<C>();
	}
	
	public void performA()
	{
		System.out.println("AImpl.performA");
	}

	public Token getToken()
	{
		return new TokenImpl((int)(Math.random() * 100.0));
	}
}
