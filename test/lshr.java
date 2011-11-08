class lshr {
  public static void check(boolean b) throws Exception {
    if (!b) throw new Exception("Check failed!");
  }

  public static void main(String[] args) throws Exception {
    long l = 1;
    check(l == 1);
    check(l >> 10 == 0);
    check(l >> 32 == 0);
    check(l >> 64 == 1);
    check(l >> 128 == 1);
    check(l >> -10 == 0);
  }
}
