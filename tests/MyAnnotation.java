import java.lang.annotation.*;
@Retention(value=RetentionPolicy.RUNTIME)
public @interface MyAnnotation {
	int property();
	int[] primes() default {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
	String property3() default "Default value";
	Class property4();
}
