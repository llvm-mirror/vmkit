package j3mgr;

import java.util.Set;
import j3.J3Mgr;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;
import org.osgi.framework.BundleEvent;
import org.osgi.framework.BundleListener;
import org.osgi.framework.ServiceReference;
import org.osgi.service.packageadmin.PackageAdmin;

public class J3MgrImpl
	implements J3Mgr, BundleListener
{
	BundleContext context;
	
	public void open(BundleContext bundleContext)
	{
		context = bundleContext;
		
		context.addBundleListener(this);
	}

	public void close()
	{
		context.removeBundleListener(this);
		context = null;
	}

	public void setBundleStaleReferenceCorrected(
		long bundleID, boolean corrected) throws Throwable
	{
		j3.vm.OSGi.setBundleStaleReferenceCorrected(bundleID, corrected);
		
		if (!corrected || (context.getBundle(bundleID) != null))
			return;	// Bundle ignored, or still installed
		
		// Inexistent bundle to be corrected, probably uninstalled,
		// Notify the VM now.
		j3.vm.OSGi.notifyBundleUninstalled(bundleID);
		refreshFramework();
	}

	public boolean isBundleStaleReferenceCorrected(
		long bundleID) throws Throwable
	{
		return j3.vm.OSGi.isBundleStaleReferenceCorrected(bundleID);
	}

	public void bundleChanged(BundleEvent event)
	{
		int type = event.getType();
		if (type == BundleEvent.UNINSTALLED) {
			try {
				refreshFramework();
				j3.vm.OSGi.notifyBundleUninstalled(event.getBundle().getBundleId());
			} catch (Throwable e) {}
		} else if (type == BundleEvent.UPDATED) {
			try {
				refreshFramework();
				j3.vm.OSGi.notifyBundleUpdated(event.getBundle().getBundleId());
			} catch (Throwable e) {}
		}
	}
	
	void refreshFramework()
	{
		ServiceReference pkgAdminRef =
			(ServiceReference)context.getServiceReference(
				"org.osgi.service.packageadmin.PackageAdmin");
		PackageAdmin pkgAdmin = (PackageAdmin)context.getService(pkgAdminRef);
		pkgAdmin.refreshPackages(null);
		context.ungetService(pkgAdminRef);
	}
	
	// THE FOLLOWING METHODS ARE DEBUGGING HELPERS
	// THEY SHOULD BE REMOVED IN PRODUCTION
	
	public void setBundleStaleReferenceCorrected(
		String bundleNameOrID, String corrected) throws Throwable
	{
		long bundleID = getBundleID(bundleNameOrID);
		if (bundleID == -1) throw new IllegalArgumentException(bundleNameOrID);

		setBundleStaleReferenceCorrected(bundleID, corrected.equals("yes"));
	}
	
	public void isBundleStaleReferenceCorrected(
		String bundleNameOrID) throws Throwable
	{
		long bundleID = getBundleID(bundleNameOrID);
		if (bundleID == -1) throw new IllegalArgumentException(bundleNameOrID);
		
		boolean value = isBundleStaleReferenceCorrected(bundleID);
		System.out.println(
			"isBundleStaleReferenceCorrected(bundleID=" + bundleID + ") = " +
			(value ? "yes" : "no"));
	}

	long getBundleID(String symbolicNameOrID)
	{
		try {
			long bundleID = Long.parseLong(symbolicNameOrID);
			
			if (context.getBundle(bundleID) == null) {
				System.out.println(
					"WARNING: bundleID=" + bundleID +
					" is invalid, probably already uninstalled.");
			}
			
			return (bundleID < 0) ? -1 : bundleID;
		} catch (NumberFormatException e) {
			// This is not a bundle ID, it must be a symbolic name
		}
		
		Bundle[] bundles = context.getBundles();
		for (int i=0; i < bundles.length; ++i) {
			if (symbolicNameOrID.equals(bundles[i].getSymbolicName()))
				return bundles[i].getBundleId();
		}
		return -1;
	}

	public void dumpClassLoaderBundles() throws Throwable
	{
		j3.vm.OSGi.dumpClassLoaderBundles();
	}

	public void dumpReferencesToObject(String objectPointer) throws Throwable
	{
		int radix = 10;
		if (objectPointer.startsWith("0x")) {
			objectPointer = objectPointer.substring(2);
			radix = 16;
		}

		long obj = Long.parseLong(objectPointer, radix);
		
		dumpReferencesToObject(obj, null, 4);
		System.out.println(";");
	}
	
	public void dumpReferencesToObject(long obj, Set<Long> objects, int depth) throws Throwable
	{
		if (obj == 0) {
			System.out.print("0x0");
			return;
		}
/*		
		if (objects == null)
			objects = new TreeSet<Long>();
		
		Long Obj = new Long(obj);
		if ((depth <= 0) || objects.contains(Obj)) {
			System.out.print(j3.vm.OSGi.dumpObject(obj));
			return;	// Already dumped or depth reached
		}
		objects.add(Obj);
*/	
		if (depth <= 0) {
			System.out.print(j3.vm.OSGi.dumpObject(obj));
			return;	// Already dumped or depth reached
		}
		
		System.out.print(j3.vm.OSGi.dumpObject(obj) + "<<{");
		long[] refs = j3.vm.OSGi.getReferencesToObject(obj);
		if (refs != null && refs.length > 0) {
			dumpReferencesToObject(refs[0], objects, depth - 1);
			
			for (int i=1; i < refs.length; ++i) {
				System.out.print(",");
				dumpReferencesToObject(refs[i], objects, depth - 1);
			}
		}
		System.out.print("}");
	}
}
