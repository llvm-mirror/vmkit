package ijvm.tests.BImpl;

import java.util.ArrayList;

import ijvm.tests.A.A;
import ijvm.tests.A.Token;
import ijvm.tests.B.B;

public class BImpl
	implements B
{
	public ArrayList<A> a;
	public ArrayList<Token> tokens;
	
	public BImpl()
	{
		a = new ArrayList<A>();
		tokens = new ArrayList<Token>();
	}
	
	public void performB()
	{
		System.out.println("BImpl.performB");
	}
}
