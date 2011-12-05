public class ThreadTest extends Thread {
  public void run() {
    // Nothing
  }

  public static void main(String[] args) throws InterruptedException {
    for(int i = 0; i < 10000; ++i) {
      Thread t1 = new ThreadTest();
      Thread t2 = new ThreadTest();
      t1.start();
      t2.start();

      t1.join();
      t2.join();
    }
  }
}
