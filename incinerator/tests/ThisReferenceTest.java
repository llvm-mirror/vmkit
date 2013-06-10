public class ThisReferenceTest {
  public static void main(String[] args) {
    try {
      new ThisReferenceTest().foo();
    } catch (NullPointerException e) {
    }
  }

  public void foo() {
    Nested other = null;
    other.bar(this, 2L);
  }

  public static class Nested {
    public void bar(ThisReferenceTest other, long l) {
    }
  }
}
