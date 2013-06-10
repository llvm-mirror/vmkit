public class InstanceOfThisTest {
  public static void main(String[] args) {
    new A().maybeFoo();
    new B().maybeFoo();
    new C().maybeFoo();
  }

  static class A {
    void maybeFoo() {
      if (this instanceof B)
        ((B)this).foo();
    }
  }

  static class B extends A {
    void foo() {
      System.out.println("B!");
    }
  }

  static class C extends B {
    void foo() {
      System.out.println("C!");
    }
  }
}

