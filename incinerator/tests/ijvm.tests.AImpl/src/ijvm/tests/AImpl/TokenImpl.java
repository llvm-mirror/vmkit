package ijvm.tests.AImpl;

import java.util.ArrayList;

import ijvm.tests.A.Token;

public class TokenImpl
	implements Token
{
	static final int ChunkSize = 2 * 1024;
	static final int ChunkCount = 64;
	
	ArrayList<byte[]> BigData;
	
	public TokenImpl(int value)
	{
		BigData = new ArrayList<byte[]>();
		
		for (int i=0; i<ChunkCount; ++i) {
			byte[] chunk = new byte[ChunkSize];
		
			for (int j=0; j<ChunkSize; ++j)
				chunk[j] = (byte)(j % 256);
				
			BigData.add(chunk);
		}
	}
	
	public int getValue()
	{
		byte[] chunk = BigData.get((int)(Math.random() * ChunkCount));
		return chunk[(int)(Math.random() * ChunkSize)];
	}
}
