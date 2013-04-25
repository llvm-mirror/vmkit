import java.lang.Deprecated;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Method;


public class AnnotationClassTest {

	@MyAnnotation (property = 13, property4 = System.class)
  static class Sample {
    @Deprecated
    @MyAnnotation (property=5, property4=String.class)
    public int x;
    
    @MyAnnotation (property=7, property4=String.class)
    public int y() {
    	return x*x;
    }
  }

  public static void main(String[] args) throws Exception {
    Field f = Sample.class.getField("x");
    Method m = Sample.class.getMethod("y");
    Sample sample = new Sample();
    sample.x = 14;
    
    Annotation [] fAnno = f.getDeclaredAnnotations();
    Annotation [] mAnno = m.getAnnotations();
    Annotation [] cAnno = Sample.class.getDeclaredAnnotations();

    MyAnnotation xx = (MyAnnotation)f.getAnnotation(MyAnnotation.class);
		MyAnnotation yy = (MyAnnotation)m.getAnnotation(MyAnnotation.class);
		
		

    check(f.getInt(sample) == 14);
    f.setInt(sample, 17);
    check(f.getInt(sample) == 17);
    check(xx != null);
    check(yy != null);
    check(mAnno.length == 1);
		check(fAnno.length == 2);
		check(cAnno != null && cAnno.length == 1 && cAnno[0]!= null);
    //int s = yy.property();
    //check(s == 5);
    
  }

  private static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Test failed!!!");
  }
}
