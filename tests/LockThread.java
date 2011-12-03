// Some madness with threads and locks.  Shouldn't cause VM to crash!
public class LockThread extends Thread {
  static Object Lock;
  static long x = 0;
  static final long max = 1000000;
  public void run() {
    while(x < max)
      synchronized(Lock) {
        ++x;
        if (x % 100 == 0)
          ++x;
      }
  }

  public static void main(String[] args) {
    Lock = new Object();
    for(int iter = 0; iter < 100; ++iter) {
      x = 0;
      for(int i = 0; i < 100; ++i) {
        new LockThread().start();
      }
      while(x < max)
        Lock = new Object();
      try {
        sleep(100);
      } catch(InterruptedException ignored);
    }
  }
}
