class LshlTest {
  public static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Check failed!");
  }

  public static void main(String[] args) throws Exception {
    long l = 1;
    check(l == 1);
    check(l << 10 == 1024);
    check(l << 32 == 4294967296L);
    check(l << 64 == 1);
    check(l << 128 == 1);
    check(l << -10 == 18014398509481984L);
  }
}
