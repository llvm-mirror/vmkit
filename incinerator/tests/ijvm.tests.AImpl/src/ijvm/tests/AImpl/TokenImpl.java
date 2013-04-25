package ijvm.tests.AImpl;

import ijvm.tests.A.Token;

public class TokenImpl
	implements Token
{
	int value;
	
	public TokenImpl(int value)
	{
		this.value = value;
	}
	
	public int getValue()
	{
		return value;
	}
}
