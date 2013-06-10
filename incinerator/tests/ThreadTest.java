public class ThreadTest extends Thread {
  public void run() {
    // Nothing
  }

  public static void main(String[] args) throws InterruptedException {
    int count = 10000;
    try {
    	count = Integer.parseInt(args[0]);
    }
    catch (Exception e) {
    }
    for(int i = 0; i < count ; ++i) {
      Thread t1 = new ThreadTest();
      Thread t2 = new ThreadTest();
      t1.start();
      t2.start();

      t1.join();
      t2.join();
    }
  }
}
