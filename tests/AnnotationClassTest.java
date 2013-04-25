import java.lang.Deprecated;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;

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
    public int x;
  }

  public static void main(String[] args) throws Exception {
    Field f = Sample.class.getField("x");
    Sample sample = new Sample();
    sample.x = 14;

    Annotation xx = f.getAnnotation(Deprecated.class);
    //Annotation[] a = f.getDeclaredAnnotations();

    check(f.getInt(sample) == 14);
    f.setInt(sample, 17);
    check(f.getInt(sample) == 17);
    check(xx.annotationType().getCanonicalName().equals("java.lang.Deprecated"));
  }

  private static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Test failed!!!");
  }
}
