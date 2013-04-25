package ijvm.tests.staticexposer;

public class StaticExposerImpl
	implements StaticExposer
{
	public String getStaticString()
	{
		return theStaticString;
	}

	public Integer getStaticInteger()
	{
		return theStaticInteger;
	}
}
