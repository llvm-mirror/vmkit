import java.lang.Thread;

public class FieldWriteTest {
  static class FieldWriterThread extends Thread {
    boolean val = false;

    FieldWriterThread() { super(); }

    public void run() {
      val = true;
    }
  }

  public static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Check failed!");
  }

  public static void main(String[] args) throws Exception {
    // First time passes, the rest fail.
    int fail = 0;
    for (int i = 0; i < 100; ++i) {
      FieldWriterThread t = new FieldWriterThread();
      t.start();
      t.join(); // Synchronization point!
      if (!t.val) ++fail;
    }
    if (fail > 0)
      System.out.println("Failed checks: " + fail);
    check(fail == 0);
  }
}
