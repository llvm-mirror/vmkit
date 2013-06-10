package resurrection;

public class resurrected
{
	protected void finalize()
	{
		System.out.println("Finalize invoked, resurrect this object.");
		
		resurrect.objRef = this;
	}
}
