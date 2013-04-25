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
    for (int i = 0; i < 100; ++i) {
      FieldWriterThread t = new FieldWriterThread();
      t.start();
      check(t.isAlive());
      t.join(); // Synchronization point!
      check(!t.isAlive());
      check(t.val);
    }
  }
}
