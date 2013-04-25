import java.lang.Deprecated;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Created with IntelliJ IDEA.
 * User: inti
 * Date: 11/14/12
 * Time: 11:08 PM
 * To change this template use File | Settings | File Templates.
 */
public class AnnotationClassTest {

  static class Sample {
    @Deprecated
    @MyAnnotation (property=5)
    public int x;
    
    @MyAnnotation (property=5)
    public int y() {
    	return x*x;
    }
  }

  public static void main(String[] args) throws Exception {
    Field f = Sample.class.getField("x");
    Method m = Sample.class.getMethod("y");
    Sample sample = new Sample();
    sample.x = 14;
    
    f.getDeclaredAnnotations();

    MyAnnotation xx = (MyAnnotation)f.getAnnotation(MyAnnotation.class);
		MyAnnotation yy = (MyAnnotation)f.getAnnotation(MyAnnotation.class);
		
		

    check(f.getInt(sample) == 14);
    f.setInt(sample, 17);
    check(f.getInt(sample) == 17);
    check(xx != null);
    check(yy != null);
    //int s = yy.property();
    //check(s == 5);
  }

  private static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Test failed!!!");
  }
}
