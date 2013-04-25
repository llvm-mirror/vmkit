package ijvm.tests.staticexposer;

public interface StaticExposer
{
	public String theStaticString = "Hello World Two";
	public Integer theStaticInteger = new Integer((int)(Math.random() * 1000.0));
	
	public String getStaticString();
	public Integer getStaticInteger();
}
