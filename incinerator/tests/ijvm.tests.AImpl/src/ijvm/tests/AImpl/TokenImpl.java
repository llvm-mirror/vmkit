package ijvm.tests.AImpl;

import ijvm.tests.A.Token;

public class TokenImpl
	implements Token
{
	static final int BigDataSize = 2 * 1024 * 1024;
	
	byte[] BigData;
	
	public TokenImpl(int value)
	{
		BigData = new byte[BigDataSize];
		for (int i=0; i<BigDataSize; ++i)
			BigData[i] = (byte)(i % 256);
	}
	
	public int getValue()
	{
		return BigData[ (int)(Math.random() * BigDataSize) ];
	}
}
