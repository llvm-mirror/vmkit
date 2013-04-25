package ijvm.tests.AImpl;

import ijvm.tests.A.A;
import ijvm.tests.A.Token;

public class AImpl
	implements A
{
	public void performA()
	{
		System.out.println("AImpl.performA");
	}

	public Token getToken()
	{
		return new TokenImpl((int)(Math.random() * 100.0));
	}
}
