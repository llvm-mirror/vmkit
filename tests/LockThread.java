// Allocate too many fat locks, crash VM
public class LockThread extends Thread {
  static Object Lock;
  static long x = 0;
  static final long max = 1000;
  public void run() {
    while(x < max)
      synchronized(Lock) {
        ++x;
      }
  }

  public static void main(String[] args) {
    for(int i = 0; i < 2048; ++i) {
      Lock = new Object();

      Thread t1 = new LockThread();
      Thread t2 = new LockThread();
      t1.start();
      t2.start();

      try { t1.join(); } catch (InterruptedException ignore){}
      try { t2.join(); } catch (InterruptedException ignore){}
    }
  }
}
